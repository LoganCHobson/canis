using System;
using System.Collections.Generic;
using System.Numerics;

namespace Canis;

/// <summary>Explicit button links for scripted menus using their own input action maps.</summary>
public sealed class ButtonNavigation
{
    public sealed class Button
    {
        public string Id { get; }
        public Button? Up, Down, Left, Right;
        public bool DefaultSelected;
        public Func<bool> Enabled = () => true;
        public Action Activate = () => { };
        internal Button(string id) => Id = id;
    }

    private readonly List<Button> buttons = new();
    public Button? Selected { get; private set; }
    public bool FocusVisible { get; private set; }

    public Button Add(string id, Action activate, Func<bool>? enabled = null, bool defaultSelected = false)
    {
        var button = new Button(id) { Activate = activate, DefaultSelected = defaultSelected };
        if (enabled != null) button.Enabled = enabled;
        buttons.Add(button);
        return button;
    }

    public void Reset() { Selected = null; }
    public void UsePointer() { FocusVisible = false; }
    public void Hover(Button? button)
    {
        FocusVisible = false;
        Selected = button != null && button.Enabled() ? button : null;
    }

    public void Refresh()
    {
        if (Selected != null && Selected.Enabled()) return;
        Selected = buttons.Find(button => button.DefaultSelected && button.Enabled());
    }

    public void Move(Vector2 direction)
    {
        if (direction == Vector2.Zero) return;
        FocusVisible = true;
        bool hadSelection = Selected != null && Selected.Enabled();
        Refresh();
        if (!hadSelection || Selected == null) return;
        Button? Next(Button button) => direction.Y > 0 ? button.Up : direction.Y < 0 ? button.Down :
            direction.X < 0 ? button.Left : button.Right;
        // Disabled entries may remain in a graph (for example a client-only lobby).
        var next = Next(Selected);
        for (int visited = 0; next != null && visited < buttons.Count; visited++, next = Next(next))
            if (next.Enabled()) { Selected = next; return; }
    }

    public void Confirm()
    {
        FocusVisible = true;
        Refresh();
        Selected?.Activate();
    }
}
