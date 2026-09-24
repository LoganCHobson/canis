using System.Numerics;

namespace Canis;

/// <summary>RGBA color with floating-point channels, editable in the inspector.</summary>
public struct Color
{
    public float R;
    public float G;
    public float B;
    public float A;

    public Color(float r, float g, float b, float a = 1f)
    {
        R = r;
        G = g;
        B = b;
        A = a;
    }

    public static implicit operator Vector4(Color color) => new(color.R, color.G, color.B, color.A);
    public static implicit operator Color(Vector4 value) => new(value.X, value.Y, value.Z, value.W);
}
