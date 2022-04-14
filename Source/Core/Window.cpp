#include "Core/Window.hpp"

#include <GLFW/glfw3.h>

/** 
 * @brief Constructor
 */
Window::Window()
    : m_window(nullptr)
    , m_width(0)
    , m_height(0)
    , m_title()
{
}

/**
 * @brief Destructor
 */
Window::~Window()
{
}

/**
 * @brief Initializes the window with the specified properties.
 * @param[in] width Window width
 * @param[in] height Window height
 * @param[in] title Window title
 * @return Returns true if the initialization was successful.
 */
bool Window::Init(const int32_t& width, const int32_t& height, const std::string& title)
{
    if (m_window != nullptr)
    {
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	//glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (m_window == nullptr)
    {
        return false;
    }

    glfwMakeContextCurrent(m_window);

    m_width = width;
    m_height = height;
    m_title = title;

    return true;
}

/**
 * @brief Cleans up the resources allocated by this window.
 */
void Window::Cleanup()
{
    if (m_window != nullptr)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

/**
 * @brief Gets the window width.
 * @return Window width
 */
int32_t Window::GetWidth() const
{
    return m_width;
}

/**
 * @brief Gets the window height.
 * @return Window height
 */
int32_t Window::GetHeight() const
{
    return m_height;
}

/**
 * @brief Gets the size of the window.
 * @param[out] width Pointer to the variable that will store the window width
 * @param[out] height Pointer to the variable that will store the window height
 */
void Window::GetSize(int32_t* width, int32_t* height) const
{
    *width = m_width;
    *height = m_height;
}

/**
 * @brief Sets the size of the window.
 * @param[in] width New window width
 * @param[in] height New window height
 */
void Window::SetSize(const int32_t& width, const int32_t& height)
{
    m_width = width;
    m_height = height;
    glfwSetWindowSize(m_window, width, height);
}

/**
 * @brief Sets the title of this window.
 * @param[in] newTitle New window title
 */
void Window::SetTitle(const std::string& newTitle)
{
    m_title = newTitle;
    glfwSetWindowTitle(m_window, newTitle.c_str());
}

/**
 * @brief Gets the title of this window.
 * @return Window title
 */
std::string Window::GetTitle() const
{
    return m_title;
}

/**
 * @brief Checks whether this window is closed or not.
 * @return Returns true if this window is closed
 */
bool Window::IsClosed() const
{
    return glfwWindowShouldClose(m_window);
}

/**
 * @brief Polls all pending window events.
 */
void Window::PollEvents() const
{
    glfwPollEvents();
}

/**
 * @brief Swaps the front buffer with the back buffer.
 */
void Window::SwapBuffers() const
{
    glfwSwapBuffers(m_window);
}

/**
 * @brief Gets the reference to the GLFW window handle associated
 * with this window.
 * @return GLFW window handle for this window.
 */
GLFWwindow* Window::GetHandle() const
{
    return m_window;
}
