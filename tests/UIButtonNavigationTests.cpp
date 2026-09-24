#include <Canis/Yaml.hpp>
#include <Canis/Scripting/ManagedComponents.hpp>
#include <Canis/App.hpp>
#include <Canis/Components.hpp>
#include <Canis/Editor.hpp>
#include <Canis/InputManager.hpp>
#include <Canis/Scene.hpp>
#include <Canis/Window.hpp>
#include <Canis/ECS/Systems/UIInteractionSystem.hpp>
#include <iostream>
#include <stdexcept>
using namespace Canis;
static void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Receiver : ScriptableEntity { using ScriptableEntity::ScriptableEntity; int clicks = 0; };
int main()
{
    Window window("Button navigation tests", 320, 240, true);
    App app;
    InputManager input;
    app.scene.Init(&app, &window, &input);
    Editor editor;
    app.RegisterDefaults(editor);
    ScriptConf receiver;
    receiver.name = "Receiver";
    receiver.kind = RegistryEntryKind::Script;
    receiver.Get = [](Entity& e) -> void* { return e.GetScript<Receiver>(); };
    receiver.uiActions["Click"] = [](ScriptableEntity& script, const UIActionContext&) { ++static_cast<Receiver&>(script).clicks; };
    app.RegisterScript(receiver);
    auto owner = app.scene.CreateEntity("Receiver");
    auto* listener = owner.AddScript<Receiver>();
    auto makeButton = [&](const char* name) {
        auto e = app.scene.CreateEntity(name);
        auto& rect = *e.AddComponent<RectTransform>();
        rect.position = Vector2(1000); // Keep the pointer outside the fixture.
        rect.size = Vector2(40);
        auto& button = *e.AddComponent<UIButton>();
        button.targetEntity = owner;
        button.targetScript = "Receiver";
        button.actionName = "Click";
        return e;
    };
    Entity first = makeButton("First"), second = makeButton("Second"), disabled = makeButton("Disabled");
    first.GetComponent<UIButton>().defaultSelected = true;
    first.GetComponent<UIButton>().down = disabled;
    disabled.GetComponent<UIButton>().active = false;
    disabled.GetComponent<UIButton>().down = second;
    second.GetComponent<UIButton>().up = first;
    UIInteractionSystem system;
    system.scene = &app.scene; system.window = &window; system.inputManager = &input;
    auto frame = [&] { system.Update(app.scene.GetRegistry(), .016f); input.BeginSyntheticInputFrame(); };
    auto key = [&](unsigned int code, bool down) { input.SetSyntheticKey(code, down); frame(); };
    input.mouse = Vector2(0); input.mouseRel = Vector2(0);
    key(Key::DOWN, true);
    Check(first.GetComponent<UIButton>().hovered, "First direction must acquire default focus");
    key(Key::DOWN, false); key(Key::DOWN, true);
    Check(second.GetComponent<UIButton>().hovered && !first.GetComponent<UIButton>().hovered, "Disabled link was not skipped");
    key(Key::DOWN, false); key(Key::RETURN, true);
    Check(second.GetComponent<UIButton>().pressed && listener->clicks == 0, "Confirm should press before activating");
    frame(); key(Key::RETURN, false);
    Check(listener->clicks == 1, "Confirm release must dispatch exactly once");
    frame(); Check(listener->clicks == 1, "Idle frame repeated activation");
    input.SetSyntheticGamepadButton(ControllerButton::DPAD_UP, true); frame();
    Check(first.GetComponent<UIButton>().hovered, "Gamepad direction failed");
    input.SetSyntheticGamepadButton(ControllerButton::DPAD_UP, false); frame();
    input.SetSyntheticGamepadButton(ControllerButton::A, true); frame();
    input.SetSyntheticGamepadButton(ControllerButton::A, false); frame();
    Check(listener->clicks == 2, "Gamepad confirmation failed");
    key(Key::RETURN, true);
    first.Destroy(); key(Key::RETURN, false);
    Check(listener->clicks == 2, "Destroyed pressed button dispatched an action");

    // Both activation paths route C# bindings through the managed attachment identity.
    Scripting::DecodeManagedComponents(YAML::Load("Canis::ManagedScripts: [{type: test.buttons, enabled: true, fields: {}}]"), owner);
    const auto token = owner.GetComponent<Scripting::ManagedComponents>().items[0]->token;
    int managedClicks = 0;
    app.scene.managedUIAction = [&](uint64_t attachment, const std::string& action) {
        Check(attachment == token && action == "Click", "Button lost managed callback identity");
        ++managedClicks;
        return true;
    };
    second.GetComponent<UIButton>().defaultSelected = true;
    second.GetComponent<UIButton>().targetScript = "CSharp:test.buttons";
    key(Key::RETURN, true); key(Key::RETURN, false);
    Check(managedClicks == 1, "Keyboard activation did not route to C#");
    auto& rect = second.GetComponent<RectTransform>();
    input.mouse = rect.GetRectMin() + rect.originOffset + rect.GetResolvedSize() * .5f + Vector2(160, 120);
    input.mouseRel = Vector2(1);
    input.SetSyntheticMouseButton(1, true); frame();
    input.mouseRel = Vector2(0);
    input.SetSyntheticMouseButton(1, false); frame();
    Check(managedClicks == 2, "Mouse activation did not route to C#");
    auto hit = app.scene.CreateEntity("Full button hit area");
    hit.AddComponent<RectTransform>()->size = Vector2(100);
    second.GetComponent<UIButton>().hitRect = hit;
    input.mouse = Vector2(160, 120); input.mouseRel = Vector2(1);
    input.SetSyntheticMouseButton(1, true); frame();
    input.mouseRel = Vector2(0); input.SetSyntheticMouseButton(1, false); frame();
    Check(managedClicks == 3, "Separate hit area did not activate the visual button");
    input.mouse = Vector2(0);

    auto canvasEntity = app.scene.CreateEntity("Action-mapped menu");
    canvasEntity.AddComponent<RectTransform>();
    auto& canvas = *canvasEntity.AddComponent<Canvas>();
    canvas.navigateAction = 11; canvas.confirmAction = 12;
    second.GetComponent<RectTransform>().parent = canvasEntity;
    Entity third = makeButton("Third");
    third.GetComponent<RectTransform>().parent = canvasEntity;
    third.GetComponent<UIButton>().targetScript = "CSharp:test.buttons";
    second.GetComponent<UIButton>().down = third;
    third.GetComponent<UIButton>().down = second;
    InputDocument document;
    document.maps = {{10, "Menu"}};
    document.actions = {
        {11, 10, "Navigate", ActionType::Axis2D, {{21, InputScheme::Gamepad, "Gamepad/LeftStick"}}},
        {12, 10, "Confirm", ActionType::Button, {{22, InputScheme::KeyboardMouse, "Keyboard/Z"}}}
    };
    auto schema = document; for (auto& action : schema.actions) action.bindings.clear();
    std::string error;
    Check(input.Actions().RegisterSchema(schema, error), error.c_str());
    Check(input.Actions().QueueDocument(document, error), error.c_str());
    input.Actions().EnableMap({10});
    auto mappedFrame = [&](float dt = .016f) { input.Actions().Evaluate(true); system.Update(app.scene.GetRegistry(), dt); input.BeginSyntheticInputFrame(); };
    mappedFrame();
    input.Actions().Record("Gamepad/LeftStick", Vector2(0, -1)); mappedFrame();
    Check(third.GetComponent<UIButton>().hovered, "Mapped stick did not follow button links");
    mappedFrame(.1f); Check(third.GetComponent<UIButton>().hovered, "Navigation repeated too soon");
    mappedFrame(.3f); Check(second.GetComponent<UIButton>().hovered, "Held mapped direction did not repeat");
    input.Actions().Record("Gamepad/LeftStick", Vector2(0)); mappedFrame();
    input.Actions().Record("Keyboard/Z", Vector2(1, 0)); mappedFrame();
    input.Actions().Record("Keyboard/Z", Vector2(0)); mappedFrame();
    Check(managedClicks == 4, "Rebound confirmation did not call the C# binding");
    canvas.navigationEnabled = false;
    input.Actions().Record("Keyboard/Z", Vector2(1, 0)); mappedFrame();
    canvas.navigationEnabled = true;
    input.Actions().Record("Keyboard/Z", Vector2(0)); mappedFrame();
    Check(managedClicks == 4, "Text editing leaked a confirmation into button navigation");
    input.Actions().Record("Keyboard/Z", Vector2(1, 0)); mappedFrame();
    input.Actions().DisableMap({10}); mappedFrame();
    Check(managedClicks == 4, "Canceled action incorrectly activated a button");
    app.scene.managedUIAction = {};

    // Scene serialization must preserve links and remap them on duplication.
    auto* conf = app.GetScriptConf("Canis::UIButton");
    second.GetComponent<UIButton>().defaultSelected = true;
    second.GetComponent<UIButton>().right = disabled;
    YAML::Node encoded;
    conf->Encode(encoded, second);
    Check(encoded["Canis::UIButton"]["targetScript"].as<std::string>() == "CSharp:test.buttons", "C# callback binding not serialized");
    Check(encoded["Canis::UIButton"]["defaultSelected"].as<bool>(), "Default selection not serialized");
    Check(encoded["Canis::UIButton"]["right"].as<UUID>() == disabled.GetUUID(), "Directional link not serialized");
    YAML::Node nodes = YAML::Load("- {Entity: 101, Name: A, 'Canis::UIButton': {defaultSelected: true, down: 102}}\n- {Entity: 102, Name: B, 'Canis::UIButton': {up: 101}}\n");
    auto loaded = app.scene.LoadEntityNodes(nodes, false);
    Check(loaded[0]->GetComponent<UIButton>().down == loaded[1], "Duplicated directional link did not resolve");
    Check(loaded[1]->GetComponent<UIButton>().up == loaded[0], "Reverse link did not resolve");
    std::cout << "Button navigation, confirmation, lifetime and serialization passed.\n";
}
