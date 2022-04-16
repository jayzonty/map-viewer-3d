#include "Core/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Constructor
 */
Camera::Camera()
	: m_fov(90.0f)
	, m_aspectRatio(1.0f)
	, m_position(0.0f)
	, m_yaw(0.0f)
	, m_pitch(0.0f)
	, m_forward(0.0f, 0.0f, -1.0f)
	, m_right(1.0f, 0.0f, 0.0f)
	, m_up(0.0f, 1.0f, 0.0f)
	, m_worldUp(0.0f, 1.0f, 0.0f)
{
	UpdateVectors();
}

/**
 * @brief Destructor
 */
Camera::~Camera()
{
}

/**
 * Sets the camera's field of view
 * @param[in] fov New field of view in degrees
 */
void Camera::SetFieldOfView(const float& fov)
{
	m_fov = fov;
}

/**
 * @brief Gets the camera's field of view
 * @return Field of view in degrees
 */
float Camera::GetFieldOfView() const
{
	return m_fov;
}

/**
 * @brief Sets the camera's aspect ratio
 * @param[in] aspectRatio New aspect ratio
 */
void Camera::SetAspectRatio(const float& aspectRatio)
{
	m_aspectRatio = aspectRatio;
}

/**
 * @brief Gets the camera's aspect ratio
 * @return Aspect ratio
 */
float Camera::GetAspectRatio() const
{
	return m_aspectRatio;
}

/**
 * @brief Sets the camera's position
 * @param[in] position New position
 */
void Camera::SetPosition(const glm::vec3& position)
{
	m_position = position;
}

/**
 * @brief Gets the camera's position
 * @return Camera's position
 */
glm::vec3 Camera::GetPosition() const
{
	return m_position;
}

/**
 * @brief Sets the camera's yaw
 * @param[in] yaw New yaw value
 */
void Camera::SetYaw(float yaw)
{
	m_yaw = yaw;
	UpdateVectors();
}

/**
 * @brief Gets the camera's yaw
 * @return Camera yaw in degrees
 */
float Camera::GetYaw() const
{
	return m_yaw;
}

/**
 * @brief Sets the camera's pitch
 * @param[in] pitch New pitch value in degrees
 */
void Camera::SetPitch(float pitch)
{
	m_pitch = pitch;
	UpdateVectors();
}

/**
 * @brief Gets the camera's pitch
 * @return Camera pitch in degrees
 */
float Camera::GetPitch() const
{
	return m_pitch;
}

/**
 * @brief Gets the forward vector
 * @return Camera's forward vector
 */
glm::vec3 Camera::GetForwardVector() const
{
	return m_forward;
}

/**
 * @brief Gets the right vector
 * @return Camera's right vector
 */
glm::vec3 Camera::GetRightVector() const
{
	return m_right;
}

/**
 * @brief Gets the up vector
 * @return Camera's up vector
 */
glm::vec3 Camera::GetUpVector() const
{
	return m_up;
}

/**
 * @brief Sets the world's up vector
 * @param[in] worldUp World up vector
 */
void Camera::SetWorldUpVector(const glm::vec3& worldUp)
{
	m_worldUp = worldUp;
	UpdateVectors();
}

/**
 * @brief Gets the world's up vector
 * @return World's up vector
 */
glm::vec3 Camera::GetWorldUpVector() const
{
	return m_worldUp;
}

/**
 * @brief Gets the view matrix
 * @return View matrix
 */
glm::mat4 Camera::GetViewMatrix() const
{
	return glm::lookAt(m_position, m_position + m_forward, m_up);
}

/**
 * @brief Gets the projection matrix
 * @return Projection matrix
 */
glm::mat4 Camera::GetProjectionMatrix() const
{
	// TODO: Make the near and far values adjustable
	return glm::perspectiveLH_ZO(glm::radians(m_fov), m_aspectRatio, 0.1f, 100.0f);
}

/**
 * @brief Updates the forward, right, and up vectors
 */
void Camera::UpdateVectors()
{
	m_forward.x = glm::cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_forward.y = glm::sin(glm::radians(m_pitch));
	m_forward.z = glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
	m_forward = glm::normalize(m_forward);

	m_right = glm::cross(m_forward, m_worldUp);
	m_right = glm::normalize(m_right);

	m_up = glm::cross(m_right, m_forward);
}
