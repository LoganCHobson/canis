using System.Numerics;
using System.Text.Json;

namespace Canis;

public readonly record struct RaycastHit(bool Hit, Entity? Entity, Vector3 Point, Vector3 Normal, float Distance);

public static class Physics
{
    /// <summary>Nearest collision; optionally exclude an entity and its transform descendants.</summary>
    public static RaycastHit Raycast(Vector3 origin, Vector3 direction, float distance, Entity? ignoreHierarchy = null)
    {
        using var result = JsonDocument.Parse(NativeBridge.Call<string>("Physics.Raycast", origin, direction, distance, ignoreHierarchy?.Handle ?? 0));
        var hit = result.RootElement;
        if (hit.ValueKind == JsonValueKind.Null)
            return default;
        static Vector3 Vector(JsonElement value) => new(value[0].GetSingle(), value[1].GetSingle(), value[2].GetSingle());
        return new(true, Entity.FromHandle(hit.GetProperty("entity").GetUInt64()), Vector(hit.GetProperty("point")),
            Vector(hit.GetProperty("normal")), hit.GetProperty("distance").GetSingle());
    }
}
