#include "Core/Input.hpp"

#include "Core/Window.hpp"

#include <cmath>

/**
 * @brief Constructor
 */
Input::Input()
	: m_pressedKeys()
	, m_releasedKeys()
	, m_heldKeys()
	, m_mousePositionX(0)
	, m_mousePositionY(0)
	, m_mouseDeltaX(0)
	, m_mouseDeltaY(0)
    , m_mouseScrollX(0)
    , m_mouseScrollY(0)
{
}

/**
 * @brief Destructor
 */
Input::~Input()
{
}

/**
 * @brief Gets the singleton instance of this class
 * @return Singleton instance of this class
 */
Input& Input::GetInstance()
{
    static Input instance;
    return instance;
}

/**
 * @brief Is the specified key just pressed during this frame?
 * @param[in] key Key to query
 * @return True if the key was just pressed during this frame. False otherwise
 */
bool Input::IsKeyPressed(Key key)
{
	return (GetInstance().m_pressedKeys.find(key) !=
		GetInstance().m_pressedKeys.end());
}

/**
 * @brief Is the specified key just released in this frame?
 * @param[in] key Key to query
 * @return True if the key was released this frame. False otherwise.
 */
bool Input::IsKeyReleased(Key key)
{
	return (GetInstance().m_releasedKeys.find(key) !=
		GetInstance().m_releasedKeys.end());
}

/**
 * @brief Is the specified key being pressed/held down?
 * @param[in] key Key to query
 * @return True if the key is being held down. False otherwise.
 */
bool Input::IsKeyDown(Key key)
{
	return (GetInstance().m_heldKeys.find(key) !=
		GetInstance().m_heldKeys.end());
}

/**
 * @brief Is the specified mouse button just pressed during this frame?
 * @param[in] button Mouse button to query
 * @return True if the mouse button was just pressed during this frame. False otherwise
 */
bool Input::IsMouseButtonPressed(Button button)
{
	return (GetInstance().m_pressedButtons.find(button) !=
		GetInstance().m_pressedButtons.end());
}

/**
 * @brief Is the specified mouse button just released during this frame?
 * @param[in] button Mouse button to query
 * @return True if the mouse button was just released during this frame. False otherwise
 */
bool Input::IsMouseButtonReleased(Button button)
{
	return (GetInstance().m_releasedButtons.find(button) !=
		GetInstance().m_releasedButtons.end());
}

/**
 * @brief Is the specified mouse button being pressed/held down?
 * @param[in] button Mouse button to query
 * @return True if the button is being held down. False otherwise.
 */
bool Input::IsMouseButtonDown(Button button)
{
	return (GetInstance().m_heldButtons.find(button) !=
		GetInstance().m_heldButtons.end());
}

/**
 * @brief Gets the mouse cursor's position in the current frame
 * @param[in] window Window
 * @param[in] mouseX Mouse cursor's x-position
 * @param[in] mouseY Mouse cursor's y-position
 */
void Input::GetMousePosition(Window *window, int32_t* mouseX, int32_t* mouseY)
{
	if (nullptr != mouseX)
	{
		*mouseX = GetInstance().m_mousePositionX;
	}
	if (nullptr != mouseY)
	{
		// TODO: This feels bad to do, but I'll settle with this for now.
		*mouseY = window->GetHeight() - GetInstance().m_mousePositionY;
	}
}

/**
 * @brief Gets the mouse cursor's position in the current frame
 * @param[in] window Window
 * @return Mouse cursor position
 */
glm::vec2 Input::GetMousePosition(Window *window)
{
	// TODO: This feels bad to do, but I'll settle with this for now.
	return { GetInstance().m_mousePositionX * 1.0f, window->GetHeight() - GetInstance().m_mousePositionY * 1.0f };
}

/**
 * @brief Gets the mouse cursor's x-position in the current frame
 * @return Mouse cursor's x-position
 */
int32_t Input::GetMouseX()
{
	return GetInstance().m_mousePositionX;
}

/**
 * @brief Gets the mouse cursor's y-position in the current frame
 * @return Mouse cursor's y-position
 */
int32_t Input::GetMouseY()
{
	return GetInstance().m_mousePositionY;
}

/**
 * @brief Gets the change in mouse cursor's position between the previous frame and the current frame
 * @param[in] mouseDeltaX Pointer to the variable where the change in x-position will be stored
 * @param[in] mouseDeltaY Pointer to the variable where the change in y-position will be stored
 */
void Input::GetMouseDelta(int32_t* mouseDeltaX, int32_t* mouseDeltaY)
{
	*mouseDeltaX = GetInstance().m_mouseDeltaX;
	*mouseDeltaY = GetInstance().m_mouseDeltaY;
}

/**
 * @brief Gets the change in mouse cursor's x-position
 * @return Amount of change in x-position
 */
int32_t Input::GetMouseDeltaX()
{
	return GetInstance().m_mouseDeltaX;
}

/**
 * @brief Gets the change in mouse cursor's y-position
 * @return Amount of change in y-position
 */
int32_t Input::GetMouseDeltaY()
{
	return GetInstance().m_mouseDeltaY;
}

/**
 * @brief Gets the mouse scrolling in the x-axis
 * @return Mouse scroll in the x-axis
 */
int32_t Input::GetMouseScrollX()
{
    return GetInstance().m_mouseScrollX;
}

/**
 * @brief Gets the mouse scrolling in the y-axis
 * @return Mouse scroll in the y-axis
 */
int32_t Input::GetMouseScrollY()
{
    return GetInstance().m_mouseScrollY;
}

/**
 * @brief Prepare the input manager for polling its new state
 */
void Input::Prepare()
{
	GetInstance().m_pressedKeys.clear();
	GetInstance().m_releasedKeys.clear();

	GetInstance().m_pressedButtons.clear();
	GetInstance().m_releasedKeys.clear();

	GetInstance().m_mouseDeltaX = 0;
	GetInstance().m_mouseDeltaY = 0;

    GetInstance().m_mouseScrollX = 0;
    GetInstance().m_mouseScrollY = 0;
}

/**
 * @brief GLFW callback function for when a key event happened
 * @param[in] window Reference to the GLFW window that received the event
 * @param[in] key Key
 * @param[in] scanCode Scan code
 * @param[in] action Action
 * @param[in] mods Modifiers
 */
void Input::KeyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		GetInstance().m_pressedKeys.insert(static_cast<Key>(key));
		GetInstance().m_heldKeys.insert(static_cast<Key>(key));
	}
	else if (action == GLFW_RELEASE)
	{
		GetInstance().m_pressedKeys.erase(static_cast<Key>(key));
		GetInstance().m_heldKeys.erase(static_cast<Key>(key));

		GetInstance().m_releasedKeys.insert(static_cast<Key>(key));
	}
}

/**
 * @brief GLFW callback function for when a mouse button event happened
 * @param[in] window Reference to the GLFW window that received the event
 * @param[in] button Mouse button
 * @param[in] action Action
 * @param[in] mods Modifiers
 */
void Input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		GetInstance().m_pressedButtons.insert(static_cast<Button>(button));
		GetInstance().m_heldButtons.insert(static_cast<Button>(button));
	}
	else if (action == GLFW_RELEASE)
	{
		GetInstance().m_pressedButtons.erase(static_cast<Button>(button));
		GetInstance().m_heldButtons.erase(static_cast<Button>(button));

		GetInstance().m_releasedButtons.insert(static_cast<Button>(button));
	}
}

/**
 * @brief GLFW callback function for when the mouse scroll wheel was
 * scrolled
 * @param[in] window Reference to the GLFW window that received the event
 * @param[in] xOffset Scroll offset in the x-axis
 * @param[in] yOffset Scroll offset in the y-axis
 */
void Input::MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    GetInstance().m_mouseScrollX = static_cast<int32_t>(xOffset);
    GetInstance().m_mouseScrollY = static_cast<int32_t>(yOffset);
}

/**
 * @brief GLFW callback function for when a mouse cursor event happened
 * @param[in] window Reference to the GLFW window that received the event
 * @param[in] xPos Mouse cursor x-position
 * @param[in] yPos Mouse cursor y-position
 */
void Input::CursorCallback(GLFWwindow* window, double xPos, double yPos)
{
	int32_t currentMouseX = static_cast<int32_t>(floor(xPos));
	int32_t currentMouseY = static_cast<int32_t>(floor(yPos));

	// At this point, m_mousePositionX and m_mousePositionY contains the
	// cursor position of the previous frame
	GetInstance().m_mouseDeltaX = currentMouseX - GetInstance().m_mousePositionX;
	GetInstance().m_mouseDeltaY = currentMouseY - GetInstance().m_mousePositionY;

	GetInstance().m_mousePositionX = currentMouseX;
	GetInstance().m_mousePositionY = currentMouseY;
}

/**
 * @brief GLFW callback function for when the mouse cursor entered or left the window
 * @param[in] window Reference to the GLFW window that received the event
 * @param[in] entered Integer indicating whether the cursor entered (1) or left (0)
 */
void Input::CursorEnterCallback(GLFWwindow* window, int entered)
{
	if (entered)
	{
		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		GetInstance().m_mousePositionX = static_cast<int32_t>(mouseX);
		GetInstance().m_mousePositionY = static_cast<int32_t>(mouseY);
	}
}
