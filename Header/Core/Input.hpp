#pragma once

#include "Core/Window.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <unordered_set>

/**
 * Input
 */
class Input
{
public:
    /**
     * Enum for keyboard keys
     */
	enum Key
	{
		UP = GLFW_KEY_UP,
		DOWN = GLFW_KEY_DOWN,
		LEFT = GLFW_KEY_LEFT,
		RIGHT = GLFW_KEY_RIGHT,

		SPACE = GLFW_KEY_SPACE,

		A = GLFW_KEY_A,
        B = GLFW_KEY_B,
        C = GLFW_KEY_C,
		D = GLFW_KEY_D,
        E = GLFW_KEY_E,
        F = GLFW_KEY_F,
        G = GLFW_KEY_G,
        H = GLFW_KEY_H,
        I = GLFW_KEY_I,
        J = GLFW_KEY_J,
        K = GLFW_KEY_K,
        L = GLFW_KEY_L,
        M = GLFW_KEY_M,
        N = GLFW_KEY_N,
        O = GLFW_KEY_O,
        P = GLFW_KEY_P,
        Q = GLFW_KEY_Q,
		R = GLFW_KEY_R,
		S = GLFW_KEY_S,
        T = GLFW_KEY_T,
        U = GLFW_KEY_U,
        V = GLFW_KEY_V,
		W = GLFW_KEY_W,
        X = GLFW_KEY_X,
        Y = GLFW_KEY_Y,
        Z = GLFW_KEY_Z,

		ESCAPE = GLFW_KEY_ESCAPE
	};
	
    /**
     * Enum for mouse buttons
     */
	enum Button
	{
		LEFT_MOUSE = GLFW_MOUSE_BUTTON_LEFT,
		MIDDLE_MOUSE = GLFW_MOUSE_BUTTON_MIDDLE,
		RIGHT_MOUSE = GLFW_MOUSE_BUTTON_RIGHT
	};

private:
    /**
     * Set of keys that are pressed in the current frame
     */
	std::unordered_set<Key> m_pressedKeys;

    /**
     * Set of keys that were released in the current frame
     */
	std::unordered_set<Key> m_releasedKeys;

    /**
     * Set of keys that are held down in the current frame
     */
	std::unordered_set<Key> m_heldKeys;

    /**
     * Set of mouse buttons that are pressed in the current frame
     */
	std::unordered_set<Button> m_pressedButtons;

    /**
     * Set of mouse buttons that were released in the current frame
     */
	std::unordered_set<Button> m_releasedButtons;

    /**
     * Set of mouse button that are held down in the current frame
     */
	std::unordered_set<Button> m_heldButtons;

    /**
     * Mouse cursor's x-position
     */
	int32_t m_mousePositionX;

    /**
     * Mouse cursor's y-position
     */
	int32_t m_mousePositionY;

    /**
     * Mouse cursor's change in x-position
     */
	int32_t m_mouseDeltaX;

    /**
     * Mouse cursor's change in y-position
     */
	int32_t m_mouseDeltaY;

    /**
     * Mouse scroll X
     */
    int32_t m_mouseScrollX;

    /**
     * Mouse scroll Y
     */
    int32_t m_mouseScrollY;

    /**
     * @brief Constructor
     */
	Input();

public:
    /* Delete copy constructor */
    Input(const Input&) = delete;

    /* Delete assignment operator */
    void operator=(const Input&) = delete;

    /**
     * @brief Destructor
     */
	~Input();

    /**
     * @brief Gets the singleton instance of this class
     * @return Singleton instance of this class
     */
    static Input& GetInstance();

    /**
     * @brief Is the specified key just pressed during this frame?
     * @param[in] key Key to query
     * @return True if the key was just pressed during this frame. False otherwise
     */
	static bool IsKeyPressed(Key key);

    /**
     * @brief Is the specified key just released in this frame?
     * @param[in] key Key to query
     * @return True if the key was released this frame. False otherwise.
     */
	static bool IsKeyReleased(Key key);

    /**
     * @brief Is the specified key being pressed/held down?
     * @param[in] key Key to query
     * @return True if the key is being held down. False otherwise.
     */
	static bool IsKeyDown(Key key);

    /**
     * @brief Is the specified mouse button just pressed during this frame?
     * @param[in] button Mouse button to query
     * @return True if the mouse button was just pressed during this frame. False otherwise
     */
    static bool IsMouseButtonPressed(Button button);

    /**
     * @brief Is the specified mouse button just released during this frame?
     * @param[in] button Mouse button to query
     * @return True if the mouse button was just released during this frame. False otherwise
     */
    static bool IsMouseButtonReleased(Button button);

    /**
     * @brief Is the specified mouse button being pressed/held down?
     * @param[in] button Mouse button to query
     * @return True if the button is being held down. False otherwise.
     */
    static bool IsMouseButtonDown(Button button);

    /**
     * @brief Gets the mouse cursor's position in the current frame
     * @param[in] window Window
     * @param[in] mouseX Mouse cursor's x-position
     * @param[in] mouseY Mouse cursor's y-position
     */
	static void GetMousePosition(Window *window, int32_t* mouseX, int32_t* mouseY);

    /**
     * @brief Gets the mouse cursor's position in the current frame
     * @param[in] window Window
     * @return Mouse cursor position
     */
    static glm::vec2 GetMousePosition(Window *window);

    /**
     * @brief Gets the mouse cursor's x-position in the current frame
     * @return Mouse cursor's x-position
     */
	static int32_t GetMouseX();

    /**
     * @brief Gets the mouse cursor's y-position in the current frame
     * @return Mouse cursor's y-position
     */
	static int32_t GetMouseY();

    /**
     * @brief Gets the change in mouse cursor's position between the previous frame and the current frame
     * @param[in] mouseDeltaX Pointer to the variable where the change in x-position will be stored
     * @param[in] mouseDeltaY Pointer to the variable where the change in y-position will be stored
     */
	static void GetMouseDelta(int32_t* mouseDeltaX, int32_t* mouseDeltaY);

    /**
     * @brief Gets the change in mouse cursor's x-position
     * @return Amount of change in x-position
     */
	static int32_t GetMouseDeltaX();

    /**
     * @brief Gets the change in mouse cursor's y-position
     * @return Amount of change in y-position
     */
	static int32_t GetMouseDeltaY();

    /**
     * @brief Gets the mouse scrolling in the x-axis
     * @return Mouse scroll in the x-axis
     */
    static int32_t GetMouseScrollX();

    /**
     * @brief Gets the mouse scrolling in the y-axis
     * @return Mouse scroll in the y-axis
     */
    static int32_t GetMouseScrollY();

    /**
     * @brief Prepare the input manager for polling its new state
     */
	static void Prepare();

    /**
     * @brief GLFW callback function for when a key event happened
     * @param[in] window Reference to the GLFW window that received the event
     * @param[in] key Key
     * @param[in] scanCode Scan code
     * @param[in] action Action
     * @param[in] mods Modifiers
     */
	static void KeyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods);

    /**
     * @brief GLFW callback function for when a mouse button event happened
     * @param[in] window Reference to the GLFW window that received the event
     * @param[in] button Mouse button
     * @param[in] action Action
     * @param[in] mods Modifiers
     */
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    /**
     * @brief GLFW callback function for when the mouse scroll wheel was
     * scrolled
     * @param[in] window Reference to the GLFW window that received the event
     * @param[in] xOffset Scroll offset in the x-axis
     * @param[in] yOffset Scroll offset in the y-axis
     */
    static void MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

    /**
     * @brief GLFW callback function for when a mouse cursor event happened
     * @param[in] window Reference to the GLFW window that received the event
     * @param[in] xPos Mouse cursor x-position
     * @param[in] yPos Mouse cursor y-position
     */
	static void CursorCallback(GLFWwindow* window, double xPos, double yPos);

    /**
     * @brief GLFW callback function for when the mouse cursor entered or left the window
     * @param[in] window Reference to the GLFW window that received the event
     * @param[in] entered Integer indicating whether the cursor entered (1) or left (0)
     */
	static void CursorEnterCallback(GLFWwindow* window, int entered);
};
