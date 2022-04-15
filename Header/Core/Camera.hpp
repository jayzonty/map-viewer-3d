#pragma once

#include <glm/glm.hpp>

/**
 * Camera class
 */
class Camera
{
private:
	/**
	 * Vertical field of view
	 */
	float m_fov;

	/**
	 * Aspect ratio
	 */
	float m_aspectRatio;

	/**
	 * Camera position
	 */
	glm::vec3 m_position;

	/**
	 * Yaw angle
	 */
	float m_yaw;

	/**
	 * Pitch angle
	 */
	float m_pitch;

	/**
	 * Camera look direction
	 */
	glm::vec3 m_forward;

	/**
	 * Right vector
	 */
	glm::vec3 m_right;

	/**
	 * Up vector
	 */
	glm::vec3 m_up;

	/**
	 * World up vector
	 */
	glm::vec3 m_worldUp;

public:
	/**
	 * @brief Constructor
	 */
	Camera();

	/**
	 * @brief Destructor
	 */
	~Camera();

	/**
	 * Sets the camera's field of view
	 * @param[in] fov New field of view in degrees
	 */
	void SetFieldOfView(const float& fov);

	/**
	 * @brief Gets the camera's field of view
	 * @return Field of view in degrees
	 */
	float GetFieldOfView() const;

	/**
	 * @brief Sets the camera's aspect ratio
	 * @param[in] aspectRatio New aspect ratio
	 */
	void SetAspectRatio(const float& aspectRatio);

	/**
	 * @brief Gets the camera's aspect ratio
	 * @return Aspect ratio
	 */
	float GetAspectRatio() const;

	/**
	 * @brief Sets the camera's position
	 * @param[in] position New position
	 */
	void SetPosition(const glm::vec3& position);

	/**
	 * @brief Gets the camera's position
	 * @return Camera's position
	 */
	glm::vec3 GetPosition() const;

	/**
	 * @brief Sets the camera's yaw
	 * @param[in] yaw New yaw value
	 */
	void SetYaw(float yaw);

	/**
	 * @brief Gets the camera's yaw
	 * @return Camera yaw in degrees
	 */
	float GetYaw() const;

	/**
	 * @brief Sets the camera's pitch
	 * @param[in] pitch New pitch value in degrees
	 */
	void SetPitch(float pitch);

	/**
	 * @brief Gets the camera's pitch
	 * @return Camera pitch in degrees
	 */
	float GetPitch() const;

	/**
	 * @brief Gets the forward vector
	 * @return Camera's forward vector
	 */
	glm::vec3 GetForwardVector() const;

	/**
	 * @brief Gets the right vector
	 * @return Camera's right vector
	 */
	glm::vec3 GetRightVector() const;

	/**
	 * @brief Gets the up vector
	 * @return Camera's up vector
	 */
	glm::vec3 GetUpVector() const;

	/**
	 * @brief Sets the world's up vector
	 * @param[in] worldUp World up vector
	 */
	void SetWorldUpVector(const glm::vec3& worldUp);

	/**
	 * @brief Gets the world's up vector
	 * @return World's up vector
	 */
	glm::vec3 GetWorldUpVector() const;

	/**
	 * @brief Gets the view matrix
	 * @return View matrix
	 */
	glm::mat4 GetViewMatrix() const;

	/**
	 * @brief Gets the projection matrix
	 * @return Projection matrix
	 */
	glm::mat4 GetProjectionMatrix() const;

private:
	/**
	 * @brief Updates the forward, right, and up vectors
	 */
	void UpdateVectors();
};
