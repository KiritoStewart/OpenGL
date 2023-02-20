#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Angel.h"

class Camera
{
public:
	Camera();
	~Camera();

	glm::mat4 getViewMatrix();
	glm::mat4 getProjectionMatrix( bool isOrtho );

	glm::mat4 lookAt(const glm::vec4& eye, const glm::vec4& at, const glm::vec4& up);

	glm::mat4 ortho(const GLfloat left, const GLfloat right,
		const GLfloat bottom, const GLfloat top,
		const GLfloat zNear, const GLfloat zFar);

	glm::mat4 perspective(const GLfloat fovy, const GLfloat aspect,
		const GLfloat zNear, const GLfloat zFar);

	glm::mat4 frustum(const GLfloat left, const GLfloat right,
		const GLfloat bottom, const GLfloat top,
		const GLfloat zNear, const GLfloat zFar);

	// 更改视见体
	void setAspect(double _aspect);
	// 增加仰角
	void addUpAngle(float _upAngle, float offsetHeight);
	// 增加旋转角
	void addRotateAngle(float _rotateAngle);
	// 增加仰角
	void setUpAngle(float _upAngle);
	// 增加旋转角
	void setRotateAngle(float _rotateAngle);
	// 设置半径
	void setRadius(float _radius);
	// 设置位移矩阵
	void setTranslation(glm::mat4 _translate);
	// 重设相机参数
	void resetCamera();

	// 获得半径
	float getRadius();
	glm::vec4 getEye();

	// 每次更改相机参数后更新一下相关的数值
	void updateCamera();
	// 处理相机的键盘操作
	void keyboard(int key, int action, int mode);

	// 模视矩阵
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	// 跟随视角位移矩阵
	glm::mat4 translation;
	glm::mat4 backViewMatrix;

	// 相机位置参数
	float radius = 0.1;
	float rotateAngle = 0.0;
	float upAngle = 0.0;
	glm::vec4 eye;
	glm::vec4 at;
	glm::vec4 up;

	// 投影参数
	#undef near
	#undef far
	float near = 0.002;
	float far = 100.0;
	// 透视投影参数
	float fov = 45.0;
	float aspect = 1.0;
	// 正交投影参数
	float scale = 1.5;

	// 判断视角是否达到极限
	bool isLimit;
	float currentRadius;
};
#endif