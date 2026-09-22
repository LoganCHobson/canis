using System.Globalization;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Canis.ScriptMetadata;

// Bake asset identity into the assembly, so players need neither sources nor PDBs.
[Generator]
public sealed class ScriptMetadataGenerator : ISourceGenerator
{
    private static readonly DiagnosticDescriptor InvalidMetadata = new(
        "CANIS001", "Invalid script metadata", "{0}", "Canis", DiagnosticSeverity.Error, true);

    public void Initialize(GeneratorInitializationContext context) { }

    public void Execute(GeneratorExecutionContext context)
    {
        var metadata = context.AdditionalFiles
            .Where(file => file.Path.EndsWith(".cs.meta", StringComparison.OrdinalIgnoreCase))
            .ToDictionary(file => Path.GetFullPath(file.Path), StringComparer.Ordinal);
        var identities = new Dictionary<INamedTypeSymbol, string>(SymbolEqualityComparer.Default);
        var output = new StringBuilder("// Generated from C# asset metadata.\n");
        foreach (var tree in context.Compilation.SyntaxTrees)
        {
            if (!metadata.TryGetValue(Path.GetFullPath(tree.FilePath) + ".meta", out var file)) continue;
            var model = context.Compilation.GetSemanticModel(tree);
            var components = tree.GetRoot(context.CancellationToken).DescendantNodes()
                .OfType<ClassDeclarationSyntax>()
                .Select(node => model.GetDeclaredSymbol(node, context.CancellationToken))
                .OfType<INamedTypeSymbol>()
                .Where(type => !type.IsAbstract && IsComponent(type) &&
                    !type.GetAttributes().Any(a => a.AttributeClass?.ToDisplayString() == "Canis.ScriptIdAttribute"))
                .Distinct<INamedTypeSymbol>(SymbolEqualityComparer.Default).ToArray();
            if (components.Length == 0) continue;
            if (components.Length > 1)
            {
                Error(context, components[0], "Keep each attachable component in its own .cs file to use its .meta UUID: " + file.Path);
                continue;
            }
            string text = file.GetText(context.CancellationToken)?.ToString() ?? "";
            string id = Scalar(text, "UUID");
            if (!ulong.TryParse(id, NumberStyles.None, CultureInfo.InvariantCulture, out ulong uuid) || uuid == 0)
            {
                Error(context, components[0], "Expected a nonzero UInt64 UUID in " + file.Path);
                continue;
            }
            id = uuid.ToString(CultureInfo.InvariantCulture);
            var type = components[0];
            if (identities.TryGetValue(type, out var existing))
            {
                if (existing != id) Error(context, type, "Partial component has conflicting .meta UUIDs: " + type.Name);
                continue;
            }
            identities.Add(type, id);
            string alias = Scalar(text, "ScriptAlias");
            output.Append("[assembly: global::Canis.ScriptAssetAttribute(typeof(")
                .Append(type.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat)).Append("), ")
                .Append(SymbolDisplay.FormatLiteral(id, true)).Append(", ")
                .Append(SymbolDisplay.FormatLiteral(alias, true)).AppendLine(")]");
        }
        context.AddSource("Canis.ScriptAssets.g.cs", output.ToString());
    }

    private static bool IsComponent(INamedTypeSymbol type)
    {
        for (var parent = type.BaseType; parent is not null; parent = parent.BaseType)
        {
            if (parent.ToDisplayString() == "Canis.NativeComponent") return false;
            if (parent.ToDisplayString() == "Canis.Component") return true;
        }
        return false;
    }

    // Canis writes these metadata fields as simple top-level YAML scalars.
    private static string Scalar(string text, string name)
    {
        foreach (string line in text.Split('\n'))
            if (line.StartsWith(name + ":", StringComparison.Ordinal))
                return line[(name.Length + 1)..].Trim().Trim('"', '\'');
        return "";
    }

    private static void Error(GeneratorExecutionContext context, INamedTypeSymbol type, string message) =>
        context.ReportDiagnostic(Diagnostic.Create(InvalidMetadata, type.Locations.FirstOrDefault(), message));
}
