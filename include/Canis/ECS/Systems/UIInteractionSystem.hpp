#pragma once
#include <Canis/Entity.hpp>
#include <Canis/Math.hpp>

#include <Canis/System.hpp>

namespace Canis
{
    class UIInteractionSystem : public System
    {
    private:
        Entity m_selectedButton = nullptr;
        Entity m_navigationPressedButton = nullptr;
        bool m_navigationFocus = false;
        Entity m_navigationCanvas = nullptr;
        Vector2 m_navigationDirection = Vector2(0.0f);
        float m_navigationRepeat = 0.0f;
        Entity m_pressedButton = nullptr;
        Entity m_focusedInputField = nullptr;
        Entity m_dragSource = nullptr;
        Entity m_hoveredDropTarget = nullptr;

    public:
        UIInteractionSystem() : System() { m_name = type_name<UIInteractionSystem>(); }

        void Create() override {}
        void Ready() override {}
        void Update(entt::registry &_registry, float _deltaTime) override;
        bool UpdateWhenPaused() const override { return true; }
    };
}
