#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include "MeshPainter.h"
#include <vector>
#include <string>
#include <stb_image.h>
#include "Skybox.h"

using namespace std;
void robot_move();
void chair_move();
void chair_back_rotate(float rotateAngle);
void chair_rotate(float rotateAngle);

// ChangeBy
// 定义一些常数参量
struct Params {
	const float chair_move_distance = 0.0005;			// 椅子每次移动的距离
	const float chair_wheel_delta_angle = 2.0;			// 轮子每帧转动的角度
	const float chair_back_delta_angle = 0.1;			// 椅子靠背的旋转角度
	const float chair_body_delta_angle = 0.2;			// 椅子旋转每次旋转的角度
	const float robot_move_distance = 0.0005;			// 人物移动的距离
	
}params;

// 设置状态结构体，记录当前的运行状态，以防冲突
struct Status {
	bool move[4] = { false, false, false, false };				// 前，后，左，右
	bool chair_rotate[4] = { false, false, false, false };		// 椅子左旋，椅子右旋，靠背向后，靠背向前
	bool CursorMode = true;										// 光标模式，DISABLED 或者 NORMAL
	bool firstMove = true;										// 是否第一次移动光标
	bool sit = false;											// 人是否坐在椅子上
	float lastX = 0, lastY = 0;									// 上一次的光标的x,y坐标
	float floor = 0.0;											// 地板的位置
	float cameraOffsetHeight;									// 相机观看中心点相对地板的垂直位移
	int currentMesh = 1;										// 对应当前观察对象，0: 椅子， 1：人物
}status;




// ChangeBy
// 定义层级模型树结构体
struct TriMeshNode {
	TriMesh* mesh;
	TriMeshNode** child;
	int childNum, index;
	TriMeshNode() {
		mesh = NULL;
		child = NULL;
		childNum = 0, index = 0;
	}
	TriMeshNode(const int c_num, TriMesh* m) {
		mesh = m;
		childNum = c_num;
		index = 0;
		if (c_num != 0) {
			child = new TriMeshNode* [c_num];
			for (int i = 0; i < c_num; i++) {
				child[i] = NULL;
			}
		}
		else
			child = NULL;
	}
	void add_child(TriMeshNode* m) {
		if(index < childNum)
			child[index++] = m;
	}
};

int WIDTH = 1200;
int HEIGHT = 1200;

int mainWindow;

Camera* camera = new Camera();
Light* light = new Light();
MeshPainter *painter = new MeshPainter();
Skybox* skybox = new Skybox();					// 定义的天空盒类
TriMeshNode* chair_tree = NULL;					// 椅子的层级树	
TriMeshNode* robot_tree = NULL;					// 机器人的层级树
TriMeshNode* sit_tree = NULL;					// 机器人坐上椅子后的层级树

// 这个用来回收和删除我们创建的物体对象
std::vector<TriMesh *> meshList;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	double ratio = (double)width / height;
	camera->setAspect(ratio);
	glViewport(0, 0, width, height);
}

// ChangeBy
// 递归实现层级建模的递归描绘
void traverse(TriMeshNode* node, glm::mat4 modelView) {
	if (node == NULL)
		return;
	modelView *= node->mesh->getModelMatrix();
	painter->drawMesh(node->mesh, node->mesh->getOpenGLObject(), light, camera, true, modelView);
	for (int i = 0; i < node->childNum; i++) {
		traverse(node->child[i], modelView);
	}
}

// ChangeBy
// 检测当前状态，并选择执行相应函数
void execute_current_order() {
	float angle;
	if (status.currentMesh) {
		robot_move();
	}
	else {
		chair_move();
		angle = (status.chair_rotate[0] + ~int(status.chair_rotate[1]) + 1) * params.chair_body_delta_angle;
		if (angle != 0)
			chair_rotate(angle);
		angle = (status.chair_rotate[2] + ~int(status.chair_rotate[3]) + 1) * params.chair_back_delta_angle;
		if (angle != 0)
			chair_back_rotate(angle);
	}
}

// 改变机器人的控制。
void changeControl() {
	string name[] = { "chair_body", "robot_body" };
	int index;
	glm::mat4 instance(1.0f);
	TriMesh* body;
	glm::vec3 translation;
	status.currentMesh += 1;
	status.currentMesh %= 2;
	index = painter->getMeshIndex(name[status.currentMesh]);
	body = painter->getMeshes()[index];
	translation = body->getTranslation();
	translation.x *= -1;
	translation.y = -status.cameraOffsetHeight;
	translation.z *= -1;
	instance = glm::translate(instance, translation);
	camera->setTranslation(instance);
}

// ChangeBy
// ############################################### chair ###################################### //
void init_chair(string vshader, string fshader) {

	glm::vec3 chair_scale(0.007, 0.007, 0.007);
	glm::mat4 instance(1.0f);

	// 绘制椅子中间部分
	TriMesh* chair_body = new TriMesh();
	chair_body->readObj("./mode/chair/chair_body.obj");
	// 设置物体的旋转位移
	chair_body->setScale(chair_scale);
	chair_body->setRotation(glm::vec3(0.0, 0.0, 0.0));

	// 局部变换矩阵
	instance = glm::scale(instance, chair_body->getScale());
	chair_body->setInitModelMatrix(instance);

	float body_height = chair_body->getHeight();
	float body_width = chair_body->getWidth();
	float body_length = chair_body->getLength();

	// 加到painter中
	painter->addMesh(chair_body, "chair_body", "./mode/chair/chair_body.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(chair_body);

	// 绘制椅子的靠背
	instance = glm::mat4(1.0f);
	TriMesh* chair_back = new TriMesh();
	chair_back->readObj("./mode/chair/chair_back.obj");
	// 设置物体的旋转位移
	chair_back->setScale(chair_scale);

	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, chair_back->getHeight() / 2, chair_back->getWidth() / 2));
	instance = glm::scale(instance, chair_back->getScale());
	chair_back->setInitModelMatrix(instance);

	chair_back->setRotation(glm::vec3(0.0, 0.0, 0.0));
	// -chair_back->getHeight() / 2
	chair_back->setTranslation(glm::vec3(0.0, 0.3 * chair_scale.x, body_width / 2.0 - 1.1 * chair_scale.x));
	// 加到painter中
	painter->addMesh(chair_back, "chair_back", "./mode/chair/chair_back.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(chair_back);

	// 绘制椅子轮子

	TriMesh* chair_wheel[4];

	for (int i = 0; i < 4; i++) {
		chair_wheel[i] = new TriMesh();
		chair_wheel[i]->readObj("./mode/chair/chair_wheel.obj"s);
		// 设置物体的旋转位移
		chair_wheel[i]->setScale(chair_scale);

		// 设置局部矩阵
		instance = glm::mat4(1.0f);
		instance = glm::scale(instance, chair_wheel[i]->getScale());
		chair_wheel[i]->setInitModelMatrix(instance);
		chair_wheel[i]->setRotation(glm::vec3(0.0, 0.0, 0.0));

		// 加到painter中
		painter->addMesh(chair_wheel[i], "chair_wheel"s + char(i+1+'0'), "./mode/chair/chair_wheel.png"s, vshader, fshader);
		// 放入meshList以便删除模型
		meshList.push_back(chair_wheel[i]);
	}

	float wheelWidth = chair_wheel[0]->getLength() / 2;
	float wheelHeight = chair_wheel[0]->getHeight() / 2;
	float wheelOffset[2][4] = { {0.0, 0.0, body_width / 2 - wheelWidth, -body_width / 2 + wheelWidth},
								{body_length / 2 - wheelWidth, -body_length / 2 + wheelWidth, 0.0, 0.0} };
	for (int i = 0; i < 4; i++) {
		chair_wheel[i]->setTranslation(glm::vec3(wheelOffset[0][i], -body_height / 2 - wheelHeight, wheelOffset[1][i]));
	}

	// 计算相对位置，使轮子落在设定的平面上
	float totalDistance = body_height / 2.0 + chair_wheel[0]->getHeight();
	float offset = totalDistance - status.floor;
	chair_body->setTranslation(glm::vec3(0.0, offset, -0.1));

	// ChangeBy
	// 构建chair的搜索树
	chair_tree = new TriMeshNode(5, chair_body);
	TriMeshNode* node = new TriMeshNode(0, chair_back);
	chair_tree->add_child(node);
	for (int i = 0; i < 4; i++) {
		node = new TriMeshNode(0, chair_wheel[i]);
		chair_tree->add_child(node);
	}

}

// 根据前后左右的按键输入，获取当前实际的运动方向
// beginDirection是指物体原本面向的方向
// followEye表示是否跟随观察视角运动,否的话默认以物体面向的方向作为前进方向
// 而arcball状态下则为物体的朝向。
// ChangeBy
glm::vec3 getMoveDirection(TriMesh* body, int beginDirection, bool followEye) {
	glm::vec3 up(0.0, 1.0, 0.0);
	glm::vec3 forward;
	if (followEye) {
		glm::vec4 eye = camera->getEye();
		forward = glm::vec3(0.0, 0.0, 0.0) - glm::vec3(eye);
	}
	else {
		float rotateAngle = body->getRotation().y;
		forward.x = beginDirection * sin(glm::radians(rotateAngle));
		forward.y = 0.0;
		forward.z = beginDirection * cos(glm::radians(rotateAngle));
	}
	//cout << "body_rotate: " << rotateAngle << "\n";
	//cout << "forwardx: " << forward.x << " forwardz: " << forward.z << "\n";
	glm::vec3 left = glm::cross(up, forward);
	glm::vec3 right = glm::cross(forward, up);
	glm::vec3 back = -forward;
	glm::vec3 direction;
	direction.y = 0.0;
	direction.x = forward.x * status.move[0] + back.x * status.move[1] +
		left.x * status.move[2] + right.x * status.move[3];
	direction.z = forward.z * status.move[0] + back.z * status.move[1] +
		left.z * status.move[2] + right.z * status.move[3];
	return direction;
}
// ChangeBy 两个函数完成椅子的移动
// 转轮子
void chair_wheel_rotate(float x, float z) {
	int index = painter->getMeshIndex("chair_wheel1");
	glm::vec3 rotation_before = painter->getMeshes()[index]->getRotation();
	glm::vec3 rotation_delta(x, 0.0, z);
	glm::vec3 rotation_after = rotation_before + rotation_delta;
	for (int i = 0; i < 4; i++) {
		painter->getMeshes()[index + i]->setRotation(rotation_after);
	}
}
// 平移椅子
void chair_move() {
	int index = painter->getMeshIndex("chair_body");
	TriMesh* mesh = painter->getMeshes()[index];
	glm::vec3 direction = getMoveDirection(mesh, -1, false);
	if (direction != glm::vec3(0.0, 0.0, 0.0)) {
		direction = glm::normalize(direction);
		chair_wheel_rotate(params.chair_wheel_delta_angle * direction.z, -params.chair_wheel_delta_angle * direction.x);
		glm::vec3 translation = mesh->getTranslation();
		translation.x += params.chair_move_distance * direction.x;
		translation.z += params.chair_move_distance * direction.z;
		mesh->setTranslation(translation);
		//设置相机跟随
		glm::mat4 instance(1.0f);
		translation.y = status.cameraOffsetHeight;
		instance = glm::translate(instance, translation * glm::vec3(-1.0, -1.0, -1.0));
		camera->setTranslation(instance);
	}
}
// 旋转椅子
// 当旋转角为0度时，默认物体的正面朝向z轴负方向。
void chair_rotate(float rotateAngle) {
	int index = painter->getMeshIndex("chair_body");
	glm::vec3 conversion = painter->getMeshes()[index]->getRotation();
	conversion.y += rotateAngle;
	if (conversion.y >= 360)
		conversion.y -= 360;
	painter->getMeshes()[index]->setRotation(conversion);
}
// 旋转椅子靠背
void chair_back_rotate(float rotateAngle) {
	int index = painter->getMeshIndex("chair_back");
	glm::vec3 conversion = painter->getMeshes()[index]->getRotation();
	conversion.x += rotateAngle;
	if (conversion.x > 75.0)
		conversion.x = 75;
	if (conversion.x < 0)
		conversion.x = 0;
	rotateAngle = conversion.x;
	painter->getMeshes()[index]->setRotation(conversion);
	if (status.sit) {
		TriMesh* mesh;
		index = painter->getMeshIndex("robotLeftLeg");
		mesh = painter->getMeshes()[index];
		conversion = mesh->getRotation();
		conversion.x = rotateAngle - 90.0;
		mesh->setRotation(conversion);

		index = painter->getMeshIndex("robotRightLeg");
		mesh = painter->getMeshes()[index];
		conversion = mesh->getRotation();
		conversion.x = rotateAngle - 90.0;
		mesh->setRotation(conversion);

		index = painter->getMeshIndex("robotLeftarm");
		mesh = painter->getMeshes()[index];
		conversion = mesh->getRotation();
		conversion.x = rotateAngle - 90.0;
		mesh->setRotation(conversion);

		index = painter->getMeshIndex("robotRightarm");
		mesh = painter->getMeshes()[index];
		conversion = mesh->getRotation();
		conversion.x = rotateAngle - 90.0;
		mesh->setRotation(conversion);

		index = painter->getMeshIndex("robotLeftleg");
		mesh = painter->getMeshes()[index];
		conversion = mesh->getRotation();
		conversion.x = 90.0 - rotateAngle;
		mesh->setRotation(conversion);

		index = painter->getMeshIndex("robotRightleg");
		mesh = painter->getMeshes()[index];
		conversion = mesh->getRotation();
		conversion.x = 90.0 - rotateAngle;
		mesh->setRotation(conversion);

	}
}

// ############################################### robot ######################################## //
void init_robot(string vshader, string fshader) {

	glm::mat4 instance = glm::mat4(1.0f);
	glm::vec3 robotScale(0.005f, 0.005f, 0.005f);

	// 机器人身体
	TriMesh* robot_body = new TriMesh();
	robot_body->readObj("./mode/robot/robot_body.obj");
	robot_body->setScale(robotScale);

	float body_height = robot_body->getHeight();
	float body_width = robot_body->getWidth();
	float body_length = robot_body->getLength();

	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, body_height / 2.0, 0.0));
	instance = glm::scale(instance, robotScale);
	robot_body->setInitModelMatrix(instance);

	robot_body->setRotation(glm::vec3(0.0, 180.0, 0.0));
	// 加到painter中
	painter->addMesh(robot_body, "robot_body", "./mode/robot/robot_body.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_body);
	instance = glm::mat4(1.0f);

	// 机器人的头部
	TriMesh* robot_head = new TriMesh();
	robot_head->readObj("./mode/robot/robot_head.obj");
	robot_head->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::scale(instance, robotScale);
	robot_head->setInitModelMatrix(instance);
	// 加到painter中
	robot_head->setTranslation(glm::vec3(0.0, body_height + robot_head->getHeight() / 2, 0.0));
	painter->addMesh(robot_head, "robot_head", "./mode/robot/robot_head.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_head);
	instance = glm::mat4(1.0f);

	// 机器人左上臂
	TriMesh* robot_left_upper_arm= new TriMesh();
	robot_left_upper_arm->readObj("./mode/robot/robotArm.obj");
	robot_left_upper_arm->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_left_upper_arm->getHeight() / 2, 0.0));
	instance = glm::scale(instance, robotScale);
	robot_left_upper_arm->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_left_upper_arm->setTranslation(glm::vec3(body_length / 2 + robot_left_upper_arm->getLength() / 2, body_height, 0.0));
	// 加到painter中
	painter->addMesh(robot_left_upper_arm, "robotLeftArm", "./mode/robot/robotArm.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_left_upper_arm);
	instance = glm::mat4(1.0f);

	// 机器人右上臂
	TriMesh* robot_right_upper_arm = new TriMesh();
	robot_right_upper_arm->readObj("./mode/robot/robotArm.obj");
	robot_right_upper_arm->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_right_upper_arm->getHeight() / 2, 0.0));
	instance = glm::scale(instance, robotScale);
	robot_right_upper_arm->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_right_upper_arm->setTranslation(glm::vec3(-body_length / 2 - robot_right_upper_arm->getLength() / 2, body_height, 0.0));
	// 加到painter中
	painter->addMesh(robot_right_upper_arm, "robotRightArm", "./mode/robot/robotArm.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_right_upper_arm);
	instance = glm::mat4(1.0f);

	// 机器人左臂关节
	TriMesh* robot_left_arm_joint = new TriMesh();
	robot_left_arm_joint->readObj("./mode/robot/robotArmJoint.obj");
	robot_left_arm_joint->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_left_upper_arm->getHeight(), 0.0));
	instance = glm::scale(instance, robotScale);
	robot_left_arm_joint->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_left_arm_joint->setTranslation(glm::vec3(0.0, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(robot_left_arm_joint, "robotLeftArmJoint", "./mode/robot/robotArmJoint.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_left_arm_joint);
	instance = glm::mat4(1.0f);

	// 机器人左下臂
	TriMesh* robot_left_lower_arm = new TriMesh();
	robot_left_lower_arm->readObj("./mode/robot/robot_arm.obj");
	robot_left_lower_arm->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_left_lower_arm->getHeight() / 2, 0.0));
	instance = glm::scale(instance, robotScale);
	robot_left_lower_arm->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_left_lower_arm->setTranslation(glm::vec3(0.0, -robot_left_upper_arm->getHeight(), 0.0));
	// 加到painter中
	painter->addMesh(robot_left_lower_arm, "robotLeftarm", "./mode/robot/robot_arm.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_left_lower_arm);
	instance = glm::mat4(1.0f);

	// 机器人右臂关节
	TriMesh* robot_right_arm_joint = new TriMesh();
	robot_right_arm_joint->readObj("./mode/robot/robotArmJoint.obj");
	robot_right_arm_joint->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_right_upper_arm->getHeight(), 0.0));
	instance = glm::scale(instance, robotScale);
	robot_right_arm_joint->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_right_arm_joint->setTranslation(glm::vec3(0.0, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(robot_right_arm_joint, "robotRightArmJoint", "./mode/robot/robotArmJoint.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_right_arm_joint);
	instance = glm::mat4(1.0f);

	// 机器人右下臂
	TriMesh* robot_right_lower_arm = new TriMesh();
	robot_right_lower_arm->readObj("./mode/robot/robot_arm.obj");
	robot_right_lower_arm->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_right_lower_arm->getHeight() / 2, 0.0));
	instance = glm::scale(instance, robotScale);
	robot_right_lower_arm->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_right_lower_arm->setTranslation(glm::vec3(0.0, -robot_right_upper_arm->getHeight(), 0.0));
	// 加到painter中
	painter->addMesh(robot_right_lower_arm, "robotRightarm", "./mode/robot/robot_arm.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_right_lower_arm);
	instance = glm::mat4(1.0f);

	// 身体与右腿关节
	TriMesh* right_upper_leg_joint = new TriMesh();
	right_upper_leg_joint->readObj("./mode/robot/robotUpperLegJoint.obj");
	right_upper_leg_joint->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(body_length / 4, 0.0, 0.0));
	instance = glm::scale(instance, robotScale);
	right_upper_leg_joint->setInitModelMatrix(instance);
	// 设置平移旋转变换
	right_upper_leg_joint->setTranslation(glm::vec3(0.0, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(right_upper_leg_joint, "robotRightUpperLegJoint", "./mode/robot/robotUpperLegJoint.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(right_upper_leg_joint);
	instance = glm::mat4(1.0f);

	// 身体与左腿关节
	TriMesh* left_upper_leg_joint = new TriMesh();
	left_upper_leg_joint->readObj("./mode/robot/robotUpperLegJoint.obj");
	left_upper_leg_joint->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(body_length / -4.0, 0.0, 0.0));
	instance = glm::scale(instance, robotScale);
	left_upper_leg_joint->setInitModelMatrix(instance);
	// 设置平移旋转变换
	left_upper_leg_joint->setTranslation(glm::vec3(0.0, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(left_upper_leg_joint, "robotLeftUpperLegJoint", "./mode/robot/robotUpperLegJoint.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(left_upper_leg_joint);
	instance = glm::mat4(1.0f);

	// 机器人左大腿
	TriMesh* robot_left_upper_leg = new TriMesh();
	robot_left_upper_leg->readObj("./mode/robot/robotLeftLeg.obj");
	robot_left_upper_leg->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_left_upper_leg->getHeight() / 2, 0.0));
	instance = glm::scale(instance, robotScale);
	robot_left_upper_leg->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_left_upper_leg->setTranslation(glm::vec3(body_length / 4, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(robot_left_upper_leg, "robotLeftLeg", "./mode/robot/robotLeftLeg.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_left_upper_leg);
	instance = glm::mat4(1.0f);

	// 左大小腿之间的关节
	TriMesh* left_lower_leg_joint = new TriMesh();
	left_lower_leg_joint->readObj("./mode/robot/robotLowerLegJoint.obj");
	left_lower_leg_joint->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_left_upper_leg->getHeight(), 0.0));
	instance = glm::scale(instance, robotScale);
	left_lower_leg_joint->setInitModelMatrix(instance);
	// 设置平移旋转变换
	left_lower_leg_joint->setTranslation(glm::vec3(0.0, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(left_lower_leg_joint, "robotLeftLowerLegJoint", "./mode/robot/robotLowerLegJoint.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(left_lower_leg_joint);
	instance = glm::mat4(1.0f);

	// 机器人左小腿
	TriMesh* robot_left_lower_leg = new TriMesh();
	robot_left_lower_leg->readObj("./mode/robot/robot_leg.obj");
	robot_left_lower_leg->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_left_lower_leg->getHeight() / 2, 0.0));
	instance = glm::scale(instance, robotScale);
	robot_left_lower_leg->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_left_lower_leg->setTranslation(glm::vec3(0.0, -robot_left_upper_leg->getHeight(), 0.0));
	// 加到painter中
	painter->addMesh(robot_left_lower_leg, "robotLeftleg", "./mode/robot/robot_leg.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_left_lower_leg);
	instance = glm::mat4(1.0f);


	// 机器人右大腿
	TriMesh* robot_right_upper_leg = new TriMesh();
	robot_right_upper_leg->readObj("./mode/robot/robotRightLeg.obj");
	robot_right_upper_leg->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_right_upper_leg->getHeight() / 2, 0.0));
	instance = glm::scale(instance, robotScale);
	robot_right_upper_leg->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_right_upper_leg->setTranslation(glm::vec3(- body_length / 4, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(robot_right_upper_leg, "robotRightLeg", "./mode/robot/robotRightLeg.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_right_upper_leg);
	instance = glm::mat4(1.0f);

	// 右大小腿之间的关节
	TriMesh* right_lower_leg_joint = new TriMesh();
	right_lower_leg_joint->readObj("./mode/robot/robotLowerLegJoint.obj");
	right_lower_leg_joint->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_right_upper_leg->getHeight(), 0.0));
	instance = glm::scale(instance, robotScale);
	right_lower_leg_joint->setInitModelMatrix(instance);
	// 设置平移旋转变换
	right_lower_leg_joint->setTranslation(glm::vec3(0.0, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(right_lower_leg_joint, "robotRightLowerLegJoint", "./mode/robot/robotLowerLegJoint.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(right_lower_leg_joint);
	instance = glm::mat4(1.0f);

	// 机器人右小腿
	TriMesh* robot_right_lower_leg = new TriMesh();
	robot_right_lower_leg->readObj("./mode/robot/robot_leg.obj");
	robot_right_lower_leg->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_right_lower_leg->getHeight() / 2, 0.0));
	instance = glm::scale(instance, robotScale);
	robot_right_lower_leg->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_right_lower_leg->setTranslation(glm::vec3(0.0, -robot_right_upper_leg->getHeight(), 0.0));
	// 加到painter中
	painter->addMesh(robot_right_lower_leg, "robotRightleg", "./mode/robot/robot_leg.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_right_lower_leg);
	instance = glm::mat4(1.0f);

	// 机器人左脚
	TriMesh* robot_left_foot = new TriMesh();
	robot_left_foot->readObj("./mode/robot/robotLeftFoot.obj");
	robot_left_foot->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(1.0 / 18 * robot_left_foot->getLength(), 
													-robot_left_foot->getHeight() / 2.0, 
													7.0 / 34 * robot_left_foot->getWidth()));
	instance = glm::scale(instance, robotScale);
	robot_left_foot->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_left_foot->setTranslation(glm::vec3(0.0, -robot_left_lower_leg->getHeight(), 0.0));
	// 加到painter中
	painter->addMesh(robot_left_foot, "robotLeftFoot", "./mode/robot/robotLeftFoot.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_left_foot);
	instance = glm::mat4(1.0f);

	// 机器人左脚关节
	TriMesh* left_foot_joint = new TriMesh();
	left_foot_joint->readObj("./mode/robot/robotLowerLegJoint.obj");
	left_foot_joint->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_left_lower_leg->getHeight(), 0.0));
	instance = glm::scale(instance, robotScale);
	left_foot_joint->setInitModelMatrix(instance);
	// 设置平移旋转变换
	left_foot_joint->setTranslation(glm::vec3(0.0, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(left_foot_joint, "robotLeftFootJoint", "./mode/robot/robotLowerLegJoint.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(left_foot_joint);
	instance = glm::mat4(1.0f);

	// 机器人右脚
	TriMesh* robot_right_foot = new TriMesh();
	robot_right_foot->readObj("./mode/robot/robotRightFoot.obj");
	robot_right_foot->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(-1.0 / 18 * robot_right_foot->getLength(), 
													-robot_left_foot->getHeight() / 2.0, 
													7.0 / 34 * robot_left_foot->getWidth()));
	instance = glm::scale(instance, robotScale);
	robot_right_foot->setInitModelMatrix(instance);
	// 设置平移旋转变换
	robot_right_foot->setTranslation(glm::vec3(0.0, -robot_right_lower_leg->getHeight(), 0.0));
	// 加到painter中
	painter->addMesh(robot_right_foot, "robotRightFoot", "./mode/robot/robotRightFoot.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(robot_right_foot);
	instance = glm::mat4(1.0f);

	// 机器人右脚关节
	TriMesh* right_foot_joint = new TriMesh();
	right_foot_joint->readObj("./mode/robot/robotLowerLegJoint.obj");
	right_foot_joint->setScale(robotScale);
	// 局部变换矩阵
	instance = glm::translate(instance, glm::vec3(0.0, -robot_right_lower_leg->getHeight(), 0.0));
	instance = glm::scale(instance, robotScale);
	right_foot_joint->setInitModelMatrix(instance);
	// 设置平移旋转变换
	right_foot_joint->setTranslation(glm::vec3(0.0, 0.0, 0.0));
	// 加到painter中
	painter->addMesh(right_foot_joint, "robotRightFootJoint", "./mode/robot/robotLowerLegJoint.png", vshader, fshader);
	// 放入meshList以便删除模型
	meshList.push_back(right_foot_joint);
	instance = glm::mat4(1.0f);

	// 计算位移，使机器人落在地板上。
	float totalDistance = robot_left_upper_leg->getHeight() + robot_left_lower_leg->getHeight() + robot_left_foot->getHeight();
	float offset = totalDistance - status.floor;
	robot_body->setTranslation(glm::vec3(0.0, offset, 0.0));
	status.cameraOffsetHeight = offset + body_height;
	instance = glm::translate(instance, glm::vec3(0.0, -status.cameraOffsetHeight, 0.0));
	camera->setTranslation(instance);
	status.cameraOffsetHeight = offset + body_height;

	// 构建body的层级树
	// 构建身体及其子节点
	robot_tree = new TriMeshNode(7, robot_body);
	TriMeshNode* node = new TriMeshNode(0, robot_head);
	TriMeshNode* left_upper_arm = new TriMeshNode(2, robot_left_upper_arm);
	TriMeshNode* right_upper_arm = new TriMeshNode(2, robot_right_upper_arm);
	TriMeshNode* left_upper_leg = new TriMeshNode(2, robot_left_upper_leg);
	TriMeshNode* right_upper_leg = new TriMeshNode(2, robot_right_upper_leg);
	robot_tree->add_child(node);
	robot_tree->add_child(left_upper_arm);
	robot_tree->add_child(right_upper_arm);
	robot_tree->add_child(left_upper_leg);
	robot_tree->add_child(right_upper_leg);
	node = new TriMeshNode(0, left_upper_leg_joint);
	robot_tree->add_child(node);
	node = new TriMeshNode(0, right_upper_leg_joint);
	robot_tree->add_child(node);

	TriMeshNode* left_lower_leg = new TriMeshNode(2, robot_left_lower_leg);
	TriMeshNode* right_lower_leg = new TriMeshNode(2, robot_right_lower_leg);
	// 构建左上臂及其子节点
	node = new TriMeshNode(0, robot_left_arm_joint);
	left_upper_arm->add_child(node);
	node = new TriMeshNode(0, robot_left_lower_arm);
	left_upper_arm->add_child(node);
	// 构建右上臂及其子节点
	node = new TriMeshNode(0, robot_right_arm_joint);
	right_upper_arm->add_child(node);
	node = new TriMeshNode(0, robot_right_lower_arm);
	right_upper_arm->add_child(node);

	// 构建左大腿及其子节点
	node = new TriMeshNode(0, left_lower_leg_joint);
	left_upper_leg->add_child(node);
	left_upper_leg->add_child(left_lower_leg);
	// 构建右大腿及其子节点
	node = new TriMeshNode(0, right_lower_leg_joint);
	right_upper_leg->add_child(node);
	right_upper_leg->add_child(right_lower_leg);

	// 构建左小腿及其子节点
	node = new TriMeshNode(0, left_foot_joint);
	left_lower_leg->add_child(node);
	node = new TriMeshNode(0, robot_left_foot);
	left_lower_leg->add_child(node);
	// 构建右小腿及其子节点
	node = new TriMeshNode(0, right_foot_joint);
	right_lower_leg->add_child(node);
	node = new TriMeshNode(0, robot_right_foot);
	right_lower_leg->add_child(node);

}
// 机器人移动
void robot_move() {
	int index = painter->getMeshIndex("robot_body");
	TriMesh* mesh = painter->getMeshes()[index];
	glm::vec3 direction = getMoveDirection(mesh, 1, true);
	if (direction != glm::vec3(0.0, 0.0, 0.0)) {
		direction = glm::normalize(direction);
		glm::vec3 rotation = mesh->getRotation();
		float angle = atan2(direction.z, direction.x) * 180 / M_PI;
		rotation.y = -angle + 90.0;
		mesh->setRotation(rotation);
		glm::vec3 translation = mesh->getTranslation();
		translation.x += params.robot_move_distance * direction.x;
		translation.z += params.robot_move_distance * direction.z;
		mesh->setTranslation(translation);

		glm::mat4 instance(1.0f);
		translation.y = status.cameraOffsetHeight;
		instance = glm::translate(instance, translation * glm::vec3(-1.0, -1.0, -1.0));
		camera->setTranslation(instance);
	}
}

// ############################################### 构建机器人坐上椅子的层级树 ######################### //
// 当人坐在椅子上的时候，以椅子为主体， 这里将身体黏在靠背的背后
// 两条大腿的话放在椅子主体下，然后对应小腿与脚接回大小腿即可
void init_sit() {
	int index;
	TriMesh* mesh, *robotRightLeg, *robotLeftLeg;
	TriMeshNode* node;
	glm::mat4 instance;
	float backRotateAngle;
	// 先建好所有的TriMeshNode,方便下面使用
	// 椅子主体
	index = painter->getMeshIndex("chair_body");
	mesh = painter->getMeshes()[index];
	TriMeshNode* chair_body = new TriMeshNode(7, mesh);
	// 椅子靠背
	index = painter->getMeshIndex("chair_back");
	mesh = painter->getMeshes()[index];
	backRotateAngle = mesh->getRotation().x;
	TriMeshNode* chair_back = new TriMeshNode(1, mesh);

	// 机器人右大腿
	index = painter->getMeshIndex("robotRightLeg");
	robotRightLeg = painter->getMeshes()[index];
	// 机器人左大腿
	index = painter->getMeshIndex("robotLeftLeg");
	robotLeftLeg = painter->getMeshes()[index];


	sit_tree = chair_body;
	chair_body->add_child(chair_back);
	index = painter->getMeshIndex("chair_wheel1");
	for (int i = 0; i < 4; i++) {
		mesh = painter->getMeshes()[index + i];
		node = new TriMeshNode(0, mesh);
		chair_body->add_child(node);
	}

	robotRightLeg->setRotation(glm::vec3(-90.0 + backRotateAngle, 0.0, 0.0));
	robotLeftLeg->setRotation(glm::vec3(-90.0 + backRotateAngle, 0.0, 0.0));
	index = painter->getMeshIndex("robotRightleg");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(90.0f - backRotateAngle, 0.0, 0.0));
	index = painter->getMeshIndex("robotLeftleg");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(90.0f - backRotateAngle, 0.0, 0.0));

	index = painter->getMeshIndex("robotRightarm");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(-90.0f + backRotateAngle, 0.0, 0.0));
	index = painter->getMeshIndex("robotLeftarm");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(-90.0f + backRotateAngle, 0.0, 0.0));



	robot_tree->mesh->setTranslation(glm::vec3(0.0, robotLeftLeg->getWidth() / 2.0, robot_tree->mesh->getWidth() / -2.0));
	robot_tree->mesh->setRotation(glm::vec3(0.0, 180.0, 0.0));
	chair_back->add_child(robot_tree);
}

// 从坐着的状态变为站着的状态
// 人物会站在椅子前面特定的位置，并将视角切换到人物
void sitToStand() {
	glm::mat4 instance(1.0f);
	int index;
	TriMesh* mesh;
	glm::vec3 chairRotation, chairTranslation;
	float height = 0;

	index = painter->getMeshIndex("chair_body");
	mesh = painter->getMeshes()[index];
	chairRotation = mesh->getRotation();
	chairTranslation = mesh->getTranslation();

	index = painter->getMeshIndex("robotLeftarm");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(0.0, 0.0, 0.0));

	index = painter->getMeshIndex("robotRightarm");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(0.0, 0.0, 0.0));

	index = painter->getMeshIndex("robotLeftLeg");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(0.0, 0.0, 0.0));
	height += mesh->getHeight();

	index = painter->getMeshIndex("robotRightLeg");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(0.0, 0.0, 0.0));

	index = painter->getMeshIndex("robotLeftleg");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(0.0, 0.0, 0.0));
	height += mesh->getHeight();

	index = painter->getMeshIndex("robotRightleg");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(0.0, 0.0, 0.0));

	index = painter->getMeshIndex("robotLeftFoot");
	mesh = painter->getMeshes()[index];
	height += mesh->getHeight();

	height -= status.floor;

	index = painter->getMeshIndex("robot_body");
	mesh = painter->getMeshes()[index];
	mesh->setRotation(glm::vec3(0.0, chairRotation.y + 180, 0.0));
	mesh->setTranslation(
		glm::vec3(chairTranslation.x + 0.1 * -sin(glm::radians(chairRotation.y)),
			height,
			chairTranslation.z + 0.1 * -cos(glm::radians(chairRotation.y))));

}

// 通过人物是否坐上椅子来改变状态
void changeSit() {
	if (!status.sit && status.currentMesh) {
		init_sit();
		changeControl();
		status.sit = 1;
	}
	else if(status.sit && !status.currentMesh){
		sitToStand();
		changeControl();
		status.sit = 0;
	}
}

// ############################################## cube ########################################### //
// 画一个正方体
void init_cube(string vshader, string fshader) {
	float k[2][8] = { {3.0, 3.0, 3.0, 0.0, 0.0, -3.0, -3.0, -3.0},
		{-3.0, 3.0, 0.0, 3.0, -3.0, -3.0, 3.0, 0.0} };
	for (int i = 0; i < 8; i++) {
		TriMesh* cube = new TriMesh();
		cube->readObj("./mode/cube/cube.obj");
		cube->setScale(glm::vec3(0.06, 0.06, 0.06));

		glm::mat4 instance(1.0f);
		instance = glm::scale(instance, cube->getScale());
		cube->setInitModelMatrix(instance);

		cube->setAmbient(glm::vec4(0.0, 0.0, 0.0, 1.0));
		cube->setDiffuse(glm::vec4(0.55, 0.55, 0.55, 1.0));
		cube->setSpecular(glm::vec4(0.70, 0.70, 0.70, 1.0));
		cube->setShininess(0.25);

		cube->setTranslation(glm::vec3(k[0][i], status.floor + cube->getHeight() / 2, k[1][i]));

		painter->addMesh(cube, "cube"s + char(i+'0'), "./mode/cube/cube.png", vshader, fshader);
		meshList.push_back(cube);
	}
	

}

// ############################################## floor ##########################################//
// 画出地板
void init_floor(string vshader, string fshader) {

	glm::mat4 instance(1.0f);
	TriMesh* floor = new TriMesh();
	floor->generateSquareRepeat(glm::vec4(1.0, 1.0, 1.0, 1.0), 100, 100);
	glm::vec3 floorScale = glm::vec3(50.0, 50.0, 50.0);
	floor->setScale(floorScale);
	instance = glm::translate(instance, glm::vec3(0.0, status.floor-0.001, 0.0));
	instance = glm::rotate(instance, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
	instance = glm::scale(instance, floorScale);
	floor->setInitModelMatrix(instance);

	painter->addMesh(floor, "floor", "./mode/floor.jpg", vshader, fshader);
	meshList.push_back(floor);
}


// ############################################## 初始化 ########################################### //
void init()
{
	std::string vshader[3], fshader[3];
	// 读取着色器并使用
	vshader[0] = "shaders/vshader.glsl";
	fshader[0] = "shaders/fshader.glsl";

	vshader[1] = "shaders/skybox.vs";
	fshader[1] = "shaders/skybox.fs";

	vshader[2] = "shaders/vcombination.glsl";
	fshader[2] = "shaders/fcombination.glsl";

	skybox->init_skybox(vshader[1], fshader[1]);
	init_chair(vshader[0], fshader[0]);
	init_cube(vshader[2], fshader[2]);
	init_robot(vshader[0], fshader[0]);
	init_floor(vshader[0], fshader[0]);

	// 设置光源位置
	light->setTranslation(glm::vec3(0.0, 1.0, -2.0));
	light->setAmbient(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 环境光
	light->setDiffuse(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 漫反射
	light->setSpecular(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 镜面反射
	light->setAttenuation(1.0, 0.045, 0.0075); // 衰减系数
	light->setFloor(status.floor);
	
	
	glClearColor(0.2, 0.2, 0.2, 1.0);
	// glClearColor(0.0, 0.0, 0.0, 1.0);
}


// ############################################## 画图 ########################################### //
void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	skybox->renderSkyBox(camera);
	// 绘制层级模型
	if (status.sit) {
		traverse(sit_tree, glm::mat4(1.0f));
	}
	else {
		traverse(robot_tree, glm::mat4(1.0f));
		traverse(chair_tree, glm::mat4(1.0f));
	}
	// 绘制立方体
	for (int i = 0; i < 8; i++) {
		TriMesh* cube = painter->getMeshes()[painter->getMeshIndex("cube"s + char('0'+i))];
		painter->drawMesh(cube, cube->getOpenGLObject(), light, camera, true, cube->getModelMatrix());
	}
	// 绘制地板
	TriMesh* floor = painter->getMeshes()[painter->getMeshIndex("floor")];
	painter->drawMesh(floor, floor->getOpenGLObject(), light, camera, false);
	execute_current_order();
	
}

// ############################################## 提示信息 ########################################### //
void printHelp()
{
	cout << "----------------------------------------------------\n";
	cout << "Previous tips:\n"
		<< "\t The current character cannot be operated. The cube beside it is used as a reference to judge its position. Only the chair can move!\n\n";
	cout << "The following are the operating instructions:  \n"
		<< "\t1. The viewing angle can be adjusted by sliding the mouse \n"
		<< "\t2. The mouse wheel adjusts the distance from the viewing angle\n"
		<< "\t3. W A S D controls the movement of the chair\n"
		<< "\t4. Q E: Control the left and right rotation of the chair respectively\n"
		<< "\t5. X C: Control the back of the chair back backward and forward respectively\n"
		<< "\t6. controls the switching of visual angle. You can switch from a character to a chair or from a chair to a character\n"
		<< "\t7. F: From the perspective of the character, press this key to let the character sit on the chair, press the character again to leave the chair, and stand in front of the chair \n"
		<< "\t8. ALT: Display mouse\n"
		<< "\t9. ESC: Exit the interface\n";

}

// ############################################## Cursor 回调 ########################################### //
// ChangeBy
// 鼠标移动回调函数
void mouse_cuosor_callback(GLFWwindow* window, double xpos, double ypos) {

	if (status.firstMove) {
		status.lastX = xpos;
		status.lastY = ypos;
		status.firstMove = false;
		return;
	}
	int width, height;
	float xoffset, yoffset, xangle, yangle;
	glfwGetWindowSize(window, &width, &height);
	xoffset = max(min(xpos - status.lastX, (double)width / 2), -(double)width / 2);
	yoffset = max(min(ypos - status.lastY, (double)height / 2), -(double)height / 2);
	xoffset = - 2 * xoffset / width;
	yoffset = 2 * yoffset / height;
	xangle = asin(xoffset) * 180 / M_PI;
	yangle = asin(yoffset) * 180 / M_PI;
	//cout << "xoffset: " << xoffset << " yoffset: " << yoffset << "\n";
	//cout << "upAngle: " << asin(yoffset) * 180 / M_PI << " rotateAngle: " << asin(xoffset) * 180 / M_PI << "\n\n";

	camera->addUpAngle(yangle, status.cameraOffsetHeight*0.98);
	camera->addRotateAngle(xangle);
	status.lastX = xpos;
	status.lastY = ypos;
}

// ############################################## Scroll 回调 ########################################### //
// ChangeBy
// 鼠标滚轮回调函数
void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	if (yoffset < 0) {
		float radius = camera->getRadius();
		radius += 0.005;
		camera->setRadius(radius);
	}
	else if (yoffset > 0) {
		float radius = camera->getRadius();
		radius -= 0.005;
		camera->setRadius(radius);
	}
}

// ############################################## keyboard ########################################### //
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	float tmp;
	int index;
	glm::vec3 conversion;
	glm::vec4 ambient;
	// press 用于一次性改变状态的操作。
	if (action == GLFW_PRESS) {
		switch (key)
		{
			case GLFW_KEY_ESCAPE: exit(EXIT_SUCCESS); break;
			// 显示鼠标指针，并无法进行镜头的操控
			case GLFW_KEY_LEFT_ALT:
				if (status.CursorMode) {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
					glfwSetCursorPosCallback(window, NULL);
					status.firstMove = true;
				}
				else {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					glfwSetCursorPosCallback(window, mouse_cuosor_callback);
				}
				status.CursorMode = !status.CursorMode;
				break;
			case GLFW_KEY_TAB:
				changeControl();
				break;
			case GLFW_KEY_F:
				changeSit();
				break;
			case GLFW_KEY_H: printHelp(); break;
			case GLFW_KEY_Q: 
				status.chair_rotate[0] = true;
				break;
			case GLFW_KEY_E:
				status.chair_rotate[1] = true;
				break;
			case GLFW_KEY_X:
				status.chair_rotate[2] = true;
				break;
			case GLFW_KEY_C:
				status.chair_rotate[3] = true;
				break;
			case GLFW_KEY_W:
				status.move[0] = true;
				break;
			case GLFW_KEY_S:
				status.move[1] = true;
				break;
			case GLFW_KEY_A:
				status.move[2] = true;
				break;
			case GLFW_KEY_D:
				status.move[3] = true;
				break;
			default:
				// camera->keyboard(key, action, mode);
				break;
		}
	}
	else if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_W:
				status.move[0] = false;
				break;
			case GLFW_KEY_S:
				status.move[1] = false;
				break;
			case GLFW_KEY_A:
				status.move[2] = false;
				break;
			case GLFW_KEY_D:
				status.move[3] = false;
				break;
			case GLFW_KEY_Q:
				status.chair_rotate[0] = false;
				break;
			case GLFW_KEY_E:
				status.chair_rotate[1] = false;
				break;
			case GLFW_KEY_X:
				status.chair_rotate[2] = false;
				break;
			case GLFW_KEY_C:
				status.chair_rotate[3] = false;
				break;
			default:
				break;
		}
	}
}

void cleanData() {
	delete camera;
	camera = NULL;

	delete light;
	light = NULL;

	painter->cleanMeshes();

	delete painter;
	painter = NULL;
	
	for (int i=0; i<meshList.size(); i++) {
		delete meshList[i];
	}
	meshList.clear();
}


int main(int argc, char** argv)
{
	// 初始化GLFW库，必须是应用程序调用的第一个GLFW函数
	glfwInit();

	// 配置GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// 配置窗口属性
#pragma execution_character_set("utf-8")
	GLFWwindow* window = glfwCreateWindow(600, 600, "2019152115guandiyang", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_cuosor_callback);
	glfwSetScrollCallback(window, mouse_scroll_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Init mesh, shaders, buffer
	init();
	// 输出帮助信息
	printHelp();
	// 启用深度测试
	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	cleanData();

	return 0;
}