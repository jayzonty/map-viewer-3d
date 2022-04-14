#ifndef WINDOW_HEADER
#define WINDOW_HEADER

#include <GLFW/glfw3.h>

#include <cstdint>
#include <string>

/**
 * Window class
 */
class Window
{
private:
    /**
     * GLFW window
     */
    GLFWwindow* m_window;

    /**
     * Window width
     */
    int32_t m_width;

    /**
     * Window height
     */
    int32_t m_height;

    /**
     * Window title
     */
    std::string m_title;

public:
    /** 
     * @brief Constructor
     */
    Window();

    /**
     * @brief Destructor
     */
    ~Window();

    /**
     * @brief Initializes the window with the specified properties.
     * @param[in] width Window width
     * @param[in] height Window height
     * @param[in] title Window title
     * @return Returns true if the initialization was successful.
     */
    bool Init(const int32_t& width, const int32_t& height, const std::string& title);

    /**
     * @brief Cleans up the resources allocated by this window.
     */
    void Cleanup();

    /**
     * @brief Gets the window width.
     * @return Window width
     */
    int32_t GetWidth() const;

    /**
     * @brief Gets the window height.
     * @return Window height
     */
    int32_t GetHeight() const;

    /**
     * @brief Gets the size of the window.
     * @param[out] width Pointer to the variable that will store the window width
     * @param[out] height Pointer to the variable that will store the window height
     */
    void GetSize(int32_t* width, int32_t* height) const;

    /**
     * @brief Sets the size of the window.
     * @param[in] width New window width
     * @param[in] height New window height
     */
    void SetSize(const int32_t& width, const int32_t& height);

    /**
     * @brief Sets the title of this window.
     * @param[in] newTitle New window title
     */
    void SetTitle(const std::string& newTitle);

    /**
     * @brief Gets the title of this window.
     * @return Window title
     */
    std::string GetTitle() const;

    /**
     * @brief Checks whether this window is closed or not.
     * @return Returns true if this window is closed
     */
    bool IsClosed() const;

    /**
     * @brief Polls all pending window events.
     */
    void PollEvents() const;

    /**
     * @brief Swaps the front buffer with the back buffer.
     */
    void SwapBuffers() const;

    /**
     * @brief Gets the reference to the GLFW window handle associated
     * with this window.
     * @return GLFW window handle for this window.
     */
    GLFWwindow* GetHandle() const;

protected:
};

#endif // WINDOW_HEADER
