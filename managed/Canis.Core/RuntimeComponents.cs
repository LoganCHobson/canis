using System.Numerics;

namespace Canis;

public sealed partial class Animator
{
    public void SetFloat(string name, float value)
    {
        Validate();
        NativeBridge.Call("Animator.SetFloat", Entity.Handle, name, value);
    }

    public float GetFloat(string name)
    {
        Validate();
        return NativeBridge.Call<float>("Animator.GetFloat", Entity.Handle, name);
    }

    public string CurrentState
    {
        get
        {
            Validate();
            return NativeBridge.Call<string>("Animator.CurrentState", Entity.Handle);
        }
    }
}

public sealed partial class UIInputField
{
    /// <summary>Request text focus. The UI system owns starting/stopping platform text input.</summary>
    public bool Focus()
    {
        Validate();
        return NativeBridge.Call<bool>("UIInputField.Focus", Entity.Handle);
    }

    public void Blur()
    {
        Validate();
        NativeBridge.Call("UIInputField.Blur", Entity.Handle);
    }

    public bool Focused
    {
        get
        {
            Validate();
            return NativeBridge.Call<bool>("UIInputField.Focused", Entity.Handle);
        }
    }

    public string Text
    {
        get
        {
            Validate();
            return NativeBridge.Call<string>("UIInputField.Text", Entity.Handle);
        }
        set
        {
            Validate();
            NativeBridge.Call("UIInputField.SetText", Entity.Handle, value);
        }
    }

    /// <summary>Append validated text, respecting allowed characters and the character limit.</summary>
    public void InsertText(string text)
    {
        Validate();
        NativeBridge.Call("UIInputField.InsertText", Entity.Handle, text);
    }

    public void Backspace()
    {
        Validate();
        NativeBridge.Call("UIInputField.Backspace", Entity.Handle);
    }
}

public sealed partial class Canvas
{
    /// <summary>Raycast the pointer (or captured mouse's crosshair) onto this world canvas.</summary>
    public bool TryGetPointer(Entity camera, out Vector2 localPoint, float maxDistance = 3f, float occlusionTolerance = .01f)
    {
        Validate();
        var point = NativeBridge.Call<Vector3>("Canvas.Pointer", Entity.Handle, camera.Handle, maxDistance, occlusionTolerance);
        localPoint = new(point.X, point.Y);
        return point.Z > 0;
    }

    /// <summary>Project a canvas-local point into top-left-origin render pixels using the last rendered camera.</summary>
    public bool TryGetScreenPoint(Vector3 localPoint, out Vector2 screenPoint)
    {
        Validate();
        var point = NativeBridge.Call<Vector3>("Canvas.ScreenPoint", Entity.Handle, localPoint);
        screenPoint = new(point.X, point.Y);
        return point.Z > 0;
    }
}
