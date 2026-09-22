#!/usr/bin/env python3
"""Compile real script assets and verify identity without source/PDB lookup."""
from pathlib import Path
import json
import shutil
import subprocess
import sys
import tempfile
from xml.sax.saxutils import escape

core, generator = map(lambda p: Path(p).resolve(), sys.argv[1:3])
with tempfile.TemporaryDirectory(prefix='canis script metadata ') as temp:
    root = Path(temp)
    project = root / 'Scripts.csproj'
    project.write_text(f'''<Project Sdk="Microsoft.NET.Sdk">
<PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework><ImplicitUsings>enable</ImplicitUsings><DebugType>none</DebugType></PropertyGroup>
<ItemGroup><Reference Include="Canis.Core" HintPath="{escape(str(core))}" />
<Analyzer Include="{escape(str(generator))}" /><AdditionalFiles Include="*.cs.meta" /></ItemGroup></Project>''')
    (root / 'Program.cs').write_text('''using System.Reflection;
using System.Text.Json;
using Canis;
var assembly = Assembly.GetExecutingAssembly();
var types = assembly.GetTypes().Where(t => t.IsSubclassOf(typeof(Component))).ToArray();
var store = typeof(Component).Assembly.GetType("Canis.ComponentStore")!;
try { store.GetMethod("Describe", BindingFlags.Static | BindingFlags.NonPublic)!.Invoke(null, new object[] { types }); }
catch (TargetInvocationException e) { Console.Error.WriteLine(e.InnerException!.Message); return 1; }
Console.WriteLine(JsonSerializer.Serialize(types.Select(t => new {
    Name = t.FullName,
    Id = store.GetMethod("Id", BindingFlags.Static | BindingFlags.NonPublic)!.Invoke(null, new object[] { t }),
    Alias = assembly.GetCustomAttributes<ScriptAssetAttribute>().SingleOrDefault(a => a.Type == t)?.LegacyId
})));
return 0;
''')
    source = root / 'Wallet.cs'
    meta = root / 'Wallet.cs.meta'
    source.write_text('public sealed class Wallet : Canis.Component { public int Balance = 100; }')
    meta.write_text('UUID: 16003035088673311249\nScriptAlias: old.wallet\n')

    def build(error=None):
        result = subprocess.run(['dotnet', 'build', str(project), '-c', 'Release', '--nologo'], capture_output=True, text=True)
        if error:
            assert result.returncode and error in result.stdout, result.stdout + result.stderr
        else:
            assert result.returncode == 0, result.stdout + result.stderr

    def run():
        return subprocess.run(['dotnet', str(root / 'bin/Release/net10.0/Scripts.dll')], capture_output=True, text=True)

    build()
    # Remove build-time inputs before launching the compiled assembly.
    source.rename(root / 'Wallet.hidden')
    meta.rename(root / 'Wallet.meta-hidden')
    result = run()
    assert result.returncode == 0, result.stderr
    assert json.loads(result.stdout) == [{'Name': 'Wallet', 'Id': '16003035088673311249', 'Alias': 'old.wallet'}]
    assert not list((root / 'bin/Release/net10.0').glob('Scripts.pdb'))
    (root / 'Wallet.hidden').unlink()
    source = root / 'RenamedWallet.cs'
    meta = root / 'RenamedWallet.cs.meta'
    (root / 'Wallet.meta-hidden').rename(meta)
    source.write_text('public sealed class RenamedWallet : Canis.Component { public int Balance = 100; }')
    build()
    assert json.loads(run().stdout)[0]['Id'] == '16003035088673311249'
    meta.write_text('UUID: 99\nScriptAlias: old.wallet\n')
    build()
    assert json.loads(run().stdout)[0]['Id'] == '99', 'Metadata-only edit was ignored'
    meta.write_text('UUID: 0\n')
    build('CANIS001')
    meta.write_text('UUID: 99\n')
    source.write_text(source.read_text() + '\npublic sealed class Other : Canis.Component {}')
    build('Keep each attachable component')
    source.write_text('public sealed class RenamedWallet : Canis.Component { public int Balance = 100; }')
    (root / 'Other.cs').write_text('public sealed class Other : Canis.Component {}')
    (root / 'Other.cs.meta').write_text('UUID: 99\n')
    build()
    result = run()
    assert result.returncode and 'Duplicate or empty script UUID' in result.stderr, result.stdout + result.stderr
    print('PASS: metadata UUID, legacy alias, source/PDB-free execution, file/class rename, metadata-only rebuild, invalid UUID, multiple components and duplicate UUID rejection.')
