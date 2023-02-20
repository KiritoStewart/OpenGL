#include "Camera.h"

Camera::Camera() { 
	updateCamera(); 
	translation = glm::mat4(1.0f);
	currentRadius = radius;
}

Camera::~Camera() {}

glm::mat4 Camera::getViewMatrix()
{
	return this->lookAt(eye, at, up);
}

glm::mat4 Camera::getProjectionMatrix(bool isOrtho)
{
	if (isOrtho) {
		return this->ortho(-scale, scale, -scale, scale, this->near, this->far);
	}
	else {
		return this->perspective(fov, aspect, this->near, this->far);
	}
}

glm::mat4 Camera::lookAt(const glm::vec4& eye, const glm::vec4& at, const glm::vec4& up)
{
	// use glm.
	glm::vec3 eye_3 = eye;
	glm::vec3 at_3 = at;
	glm::vec3 up_3 = up;

	glm::mat4 view = glm::lookAt(eye_3, at_3, up_3);

	return view * translation;
}


glm::mat4 Camera::ortho(const GLfloat left, const GLfloat right,
	const GLfloat bottom, const GLfloat top,
	const GLfloat zNear, const GLfloat zFar)
{
	glm::mat4 c = glm::mat4(1.0f);
	c[0][0] = 2.0 / (right - left);
	c[1][1] = 2.0 / (top - bottom);
	c[2][2] = -2.0 / (zFar - zNear);
	c[3][3] = 1.0;
	c[0][3] = -(right + left) / (right - left);
	c[1][3] = -(top + bottom) / (top - bottom);
	c[2][3] = -(zFar + zNear) / (zFar - zNear);

	c = glm::transpose(c);
	return c;
}

glm::mat4 Camera::perspective(const GLfloat fovy, const GLfloat aspect,
	const GLfloat zNear, const GLfloat zFar)
{
	GLfloat top = tan(fovy * M_PI / 180 / 2) * zNear;
	GLfloat right = top * aspect;

	glm::mat4 c = glm::mat4(1.0f);
	c[0][0] = zNear / right;
	c[1][1] = zNear / top;
	c[2][2] = -(zFar + zNear) / (zFar - zNear);
	c[2][3] = -(2.0 * zFar * zNear) / (zFar - zNear);
	c[3][2] = -1.0;
	c[3][3] = 0.0;

	c = glm::transpose(c);
	return c;
}

glm::mat4 Camera::frustum(const GLfloat left, const GLfloat right,
	const GLfloat bottom, const GLfloat top,
	const GLfloat zNear, const GLfloat zFar)
{
	// 任意视锥体矩阵
	glm::mat4 c = glm::mat4(1.0f);
	c[0][0] = 2.0 * zNear / (right - left);
	c[0][2] = (right + left) / (right - left);
	c[1][1] = 2.0 * zNear / (top - bottom);
	c[1][2] = (top + bottom) / (top - bottom);
	c[2][2] = -(zFar + zNear) / (zFar - zNear);
	c[2][3] = -2.0 * zFar * zNear / (zFar - zNear);
	c[3][2] = -1.0;
	c[3][3] = 0.0;

	c = glm::transpose(c);
	return c;
}


void Camera::setAspect(double _aspect)
{
	aspect = _aspect;
}

void Camera::addUpAngle(float _upAngle, float offsetHeight) {

	float limitAngle;
	if (offsetHeight / currentRadius >= sin(glm::radians(65.0)))
		limitAngle = -65.0;
	else 
		limitAngle = -asin(offsetHeight / currentRadius) * 180 / M_PI;
	upAngle += _upAngle;
	//std::cout << "current radius: " << currentRadius << " offsetHeight: " << offsetHeight << "\n";
	//std::cout << "limitAngle: " << limitAngle << "\n";
	//std::cout << "upAngle: " << upAngle << "\n\n";
	if (upAngle > 90) {
		upAngle = 90.0;
	}
	if (upAngle < limitAngle ) {
		if (upAngle <= -65.0)
			upAngle = -65.0;
		radius = offsetHeight / -sin(glm::radians(upAngle));
		isLimit = true;
		return;
	}
	radius = currentRadius;
	isLimit = false;
}
void Camera::addRotateAngle(float _rotateAngle) { rotateAngle += _rotateAngle; }
void Camera::setUpAngle(float _upAngle) { upAngle = _upAngle; }
void Camera::setRotateAngle(float _rotateAngle) { rotateAngle = _rotateAngle; }
void Camera::setTranslation(glm::mat4 _translate) { translation = _translate; }
void Camera::setRadius(float _radius) { 
	if (!isLimit) {
		radius = _radius;
		if (radius > 0.5)
			radius = 0.5;
		else if (radius < 0.05)
			radius = 0.05;
		currentRadius = radius;
	}
}
void Camera::resetCamera()
{
	radius = 0.1;
	currentRadius = 0.1;
	rotateAngle = 0.0;
	upAngle = 0.0;
	fov = 45.0;
	aspect = 1.0;
	scale = 1.5;
}
float Camera::getRadius() { return radius; }

glm::vec4 Camera::getEye() { return eye; }

void Camera::updateCamera()
{
	// @TODO: Task1 设置相机位置和方向
	float eyex = radius * cos(upAngle * M_PI / 180.0) * sin(rotateAngle * M_PI / 180.0);
	float eyey = radius * sin(upAngle * M_PI / 180.0);
	float eyez = radius * cos(upAngle * M_PI / 180.0) * cos(rotateAngle * M_PI / 180.0);
	eye = glm::vec4(eyex, eyey, eyez, 1.0);
	at = glm::vec4(0.0, 0.0, 0.0, 1.0);
	up = glm::vec4(0.0, 1.0, 0.0, 0.0);

}

// 取消相机键盘处理事件
//void Camera::keyboard(int key, int action, int mode)
//{
//	// 键盘事件处理
//	// 通过按键改变相机和投影的参数
//	if (key == GLFW_KEY_U && mode == 0x0000) {
//		rotateAngle += 5.0;
//	}
//	else if (key == GLFW_KEY_U && mode == GLFW_MOD_SHIFT) {
//		rotateAngle -= 5.0;
//	}
//	else if (key == GLFW_KEY_I && mode == 0x0000) {
//		upAngle += 5.0;
//		if (upAngle > 180)
//			upAngle = 180;
//	}
//	else if (key == GLFW_KEY_I && mode == GLFW_MOD_SHIFT) {
//		upAngle -= 5.0;
//		if (upAngle < -180)
//			upAngle = -180;
//	}
//	else if (key == GLFW_KEY_O && mode == 0x0000) {
//		radius += 0.1;
//	}
//	else if (key == GLFW_KEY_O && mode == GLFW_MOD_SHIFT) {
//		radius -= 0.1;
//	}
//	// 空格键初始化所有参数
//	else if (key == GLFW_KEY_SPACE && mode == 0x0000) {
//		radius = 0.1;
//		rotateAngle = 0.0;
//		upAngle = 0.0;
//		fov = 45.0;
//		// aspect = 1.0;
//		scale = 1.5;
//	}
//}

