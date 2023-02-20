#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include "MeshPainter.h"
#include "stb_image.h"
#include <vector>
#include <string>


int WIDTH = 800;
int HEIGHT = 800;

class MatrixStack {
	int		_index;
	int		_size;
	glm::mat4* _matrices;

public:
	MatrixStack(int numMatrices = 100) :_index(0), _size(numMatrices)
	{
		_matrices = new glm::mat4[numMatrices];
	}

	~MatrixStack()
	{
		delete[]_matrices;
	}

	void push(const glm::mat4& m) {
		assert(_index + 1 < _size);
		_matrices[_index++] = m;
	}

	glm::mat4& pop() {
		assert(_index - 1 >= 0);
		_index--;
		return _matrices[_index];
	}
};

#define White	glm::vec3(1.0, 1.0, 1.0)
#define Yellow	glm::vec3(1.0, 1.0, 0.0)
#define Green	glm::vec3(0.0, 1.0, 0.0)
#define Cyan	glm::vec3(0.0, 1.0, 1.0)
#define Magenta	glm::vec3(1.0, 0.0, 1.0)
#define Red		glm::vec3(1.0, 0.0, 0.0)
#define Black	glm::vec3(0.0, 0.0, 0.0)
#define Blue	glm::vec3(0.0, 0.0, 1.0)
#define Brown	glm::vec3(0.5, 0.5, 0.5)

struct Robot
{
	// 关节大小
	float TORSO_HEIGHT = 5.5;  //身体长度
	float TORSO_WIDTH = 4.0;   //身体直径
	float UPPER_ARM_HEIGHT = 2.0;//大臂长度
	float LOWER_ARM_HEIGHT = 2.0;//小臂长度
	float UPPER_ARM_WIDTH = 0.5; //大臂半径
	float LOWER_ARM_WIDTH = 0.5; //小臂半径
	float UPPER_LEG_HEIGHT = 2.7;//大腿长度
	float LOWER_LEG_HEIGHT = 1.5;//小腿长度
	float UPPER_LEG_WIDTH = 0.5; //大腿直径
	float LOWER_LEG_WIDTH = 0.5; //小腿直径 脚长1.1
	float HEAD_HEIGHT = 3.5;//头直径（竖直） /水平 3.0
	float HEAD_WIDTH = 6.6; //帽子长度
	float SWORD_HEIGHT = 3.0;//剑长
	float SWORD_WIDTH = 1.0; //剑宽

	// 关节角和菜单选项值
	enum {
		Torso,			// 躯干
		Head,			// 头部
		RightUpperArm,	// 右大臂
		RightLowerArm,	// 右小臂
		LeftUpperArm,	// 左大臂
		LeftLowerArm,	// 左小臂
		RightUpperLeg,	// 右大腿
		RightLowerLeg,	// 右小腿
		LeftUpperLeg,	// 左大腿
		LeftLowerLeg,	// 左小腿
		Sword,          // 剑
	};

	// 关节角大小
	GLfloat theta[11] = {
		0.0,    // Torso
		0.0,    // Head
		0.0,    // RightUpperArm
		0.0,    // RightLowerArm
		0.0,    // LeftUpperArm
		0.0,    // LeftLowerArm
		0.0,    // RightUpperLeg
		0.0,    // RightLowerLeg
		0.0,    // LeftUpperLeg
		0.0,    // LeftLowerLeg
		60.0     // Sword
	};
	int action = 0;
	//控制走路摆动速度
	int changespeed = 10;
	//控制人物大小
	float man_size = 0.008;
};
Robot robot;
// 被选中的物体
int Selected_mesh = robot.Torso;

TriMesh* fc = new TriMesh();
TriMesh* Torso = new TriMesh();
TriMesh* Head = new TriMesh();
TriMesh* RightUpperArm = new TriMesh();
TriMesh* RightLowerArm = new TriMesh();
TriMesh* LeftUpperArm = new TriMesh();
TriMesh* LeftLowerArm = new TriMesh();
TriMesh* RightUpperLeg = new TriMesh();
TriMesh* RightLowerLeg = new TriMesh();
TriMesh* LeftUpperLeg = new TriMesh();
TriMesh* LeftLowerLeg = new TriMesh();
TriMesh* mz = new TriMesh();
TriMesh* Sword = new TriMesh();

openGLObject TorsoObject;
openGLObject HeadObject;
openGLObject RightUpperArmObject;
openGLObject RightLowerArmObject;
openGLObject LeftUpperArmObject;
openGLObject LeftLowerArmObject;
openGLObject RightUpperLegObject;
openGLObject RightLowerLegObject;
openGLObject LeftUpperLegObject;
openGLObject LeftLowerLegObject;
openGLObject SwordObject;

Camera* camera = new Camera();
Light* light = new Light();
MeshPainter* painter = new MeshPainter();
MeshPainter* painter2 = new MeshPainter();
MeshPainter* painter3 = new MeshPainter();

openGLObject object;
// 这个用来回收和删除我们创建的物体对象
std::vector<TriMesh *> meshList;


double r = 0.0;
double s = 0.08;
double x_step_size = 2.0;
double z_step_size = 4.0;

void drawMesh(glm::mat4 modelMatrix, TriMesh* mesh, openGLObject object) {

	glBindVertexArray(object.vao);

	glUseProgram(object.program);

	// 父节点矩阵 * 本节点局部变换矩阵
	glUniformMatrix4fv(object.modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	glUniformMatrix4fv(object.viewLocation, 1, GL_FALSE, &camera->viewMatrix[0][0]);
	glUniformMatrix4fv(object.projectionLocation, 1, GL_FALSE, &camera->projMatrix[0][0]);
	glUniform1f(object.shadowLocation, 0);
	// 绘制
	glDrawArrays(GL_TRIANGLES, 0, mesh->getPoints().size());
}

// 躯体
void torso(glm::mat4 modelMatrix)
{
	// 本节点局部变换矩阵
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, 0.5 * robot.TORSO_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(1.8, 1.8, 1.8) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, Torso, TorsoObject);
}

// 头部
void head(glm::mat4 modelMatrix)
{
	// 本节点局部变换矩阵
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, 0.5 * robot.HEAD_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(2.0, 2.0, 2.0) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, Head, HeadObject);
}


// 左大臂
void left_upper_arm(glm::mat4 modelMatrix)
{
	// 本节点局部变换矩阵
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -0.5 * robot.UPPER_ARM_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(1.0, 0.9, 0.9) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, LeftUpperArm, LeftUpperArmObject);
}


// @TODO: 左小臂
void left_lower_arm(glm::mat4 modelMatrix)
{
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -0.5 * robot.LOWER_ARM_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(1.0, 0.8, 0.8) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, LeftLowerArm, LeftLowerArmObject);
}

// @TODO: 右大臂
void right_upper_arm(glm::mat4 modelMatrix)
{
	// 本节点局部变换矩阵
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -0.5 * robot.UPPER_ARM_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(1.0, 0.9, 0.9) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, RightUpperArm, RightUpperArmObject);
}

// @TODO: 右小臂
void right_lower_arm(glm::mat4 modelMatrix)
{
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -0.5 * robot.LOWER_ARM_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(1.0, 0.8, 0.8) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, LeftLowerArm, LeftLowerArmObject);
}

// @TODO: 左大腿
void left_upper_leg(glm::mat4 modelMatrix)
{
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -0.5 * robot.UPPER_LEG_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(1.0, 0.8, 1.0) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, LeftUpperLeg, LeftUpperLegObject);
}

// @TODO: 左小腿
void left_lower_leg(glm::mat4 modelMatrix)
{
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -0.5 * robot.LOWER_LEG_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(0.6, 0.8, 0.6) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, LeftLowerLeg, LeftLowerLegObject);
}

// @TODO: 右大腿
void right_upper_leg(glm::mat4 modelMatrix)
{
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -0.5 * robot.UPPER_LEG_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(1.0, 0.8, 1.0) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, RightUpperLeg, RightUpperLegObject);
}

// @TODO: 右小腿
void right_lower_leg(glm::mat4 modelMatrix)
{
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -0.5 * robot.LOWER_LEG_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(0.6, 0.8, 0.6) * robot.man_size);

	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	drawMesh(modelMatrix * instance, RightLowerLeg, RightLowerLegObject);
}
//剑
void sword(glm::mat4 modelMatrix)
{
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -0.5 * robot.SWORD_HEIGHT, 0.0) * robot.man_size);
	instance = glm::scale(instance, glm::vec3(3.0, 2.0, 3.0) * robot.man_size);
	drawMesh(modelMatrix * instance, Sword, SwordObject);
}


void init()
{
	std::string vshader, fshader;
	// 读取着色器并使用
#ifdef __APPLE__	// for MacOS
	vshader = "shaders/vshader_mac.glsl";
	fshader = "shaders/fshader_mac.glsl";
#else				// for Windows
	vshader = "shaders/vshader_win.glsl";
	fshader = "shaders/fshader_win.glsl";
#endif

	// 设置光源位置
	light->setTranslation(glm::vec3(10.0, -8.0, 10.0));
	light->setAmbient(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 环境光
	light->setDiffuse(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 漫反射
	light->setSpecular(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 镜面反射
	light->setAttenuation(0.5, 0.5, 0.5); // 衰减系数


	TriMesh* house = new TriMesh();
	house->setNormalize(true);
	house->readObj("./assets/1.obj");

	// 设置物体的旋转位移
	house->setTranslation(glm::vec3(0.0, 0.01, -0.3));
	house->setRotation(glm::vec3(0.0, 90.0, 0.0));
	house->setScale(glm::vec3(0.1, 0.1, 0.1));

	painter->addMesh(house, "mesh.a", "./assets/1.png", vshader, fshader);
	
	// 加到painter中
	house->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	house->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	house->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	house->setShininess(0.5);
	
	meshList.push_back(house);//程序结束后释放数据

	TriMesh* square = new TriMesh();
	// @TODO: Task2 读取娃娃模型
	square->setNormalize(true);
	square->readObj("./assets/p1.obj");

	// 设置物体的旋转位移
	square->setTranslation(glm::vec3(0.0, -0.061, 0.0));
	square->setRotation(glm::vec3(0.0, 0.0, 0.0));
	square->setScale(glm::vec3(0.4, 0.05, 0.4));
	//// 加到painter2中
	painter2->addMesh(square, "mesh.a", "./assets/p1.png", vshader, fshader);
	square->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	square->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	square->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(square);//程序结束后释放数据


	TriMesh* dg = new TriMesh();
	dg->setNormalize(true);
	dg->readObj("./assets/dg.obj");
	// 设置物体的旋转位移
	dg->setTranslation(glm::vec3(-6, 0.0, 0.0));
	dg->setRotation(glm::vec3(0.0, 0.0, 0.0));
	dg->setScale(glm::vec3(0.2, 0.2, 0.2));
	// 加到painter2中
	painter->addMesh(dg, "mesh.a", "./assets/dg.png", vshader, fshader);
	dg->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	dg->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	dg->setSpecular(glm::vec4(0.1, 0.1, 0.1, 1.0)); // 镜面反射
	meshList.push_back(dg);//程序结束后释放数据

	

	fc->setNormalize(true);
	fc->readObj("./assets/fc.obj");
	// 设置物体的旋转位移

	// 加到painter2中
	
	fc->setAmbient(glm::vec4(0.2, 1.2, 1.3, 1.0)); // 环境光
	fc->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	fc->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	painter->addMesh(fc, "mesh.a", "./assets/fc.png", vshader, fshader);
	meshList.push_back(fc);//程序结束后释放数据



	mz->setNormalize(true);
	mz->readObj("./assets/mz.obj");

	// 加到painter2中

	mz->setAmbient(glm::vec4(0.2, 1.2, 1.3, 1.0)); // 环境光
	mz->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	mz->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	painter->addMesh(mz, "mesh.a", "./assets/mz.png", vshader, fshader);
	meshList.push_back(mz);//程序结束后释放数据



	TriMesh* pool = new TriMesh();
	//读取池塘模型
	pool->setNormalize(true);
	pool->readObj("./assets/pool.obj");

	// 设置物体的旋转位移
	pool->setTranslation(glm::vec3(0.1, 0.0, -2.8));
	pool->setRotation(glm::vec3(0.0, 0.0, 0.0));
	pool->setScale(glm::vec3(0.2, 0.02, 0.2));

	// 加到painter中
	painter->addMesh(pool, "mesh.a", "./assets/pool.png", vshader, fshader);
	pool->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	pool->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	pool->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(pool);//程序结束后释放数据



	
	
	
	//绘制人物
	//身体
	Torso->setNormalize(true);
	Torso->readObj("./assets/man/body.obj");

	painter3->bindObjectAndData(Torso, TorsoObject, "./assets/man/body.png", vshader, fshader);
	Torso->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	Torso->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	Torso->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(Torso);//程序结束后释放数据

	//头
	Head->setNormalize(true);
	Head->readObj("./assets/man/head.obj");

	painter3->bindObjectAndData(Head, HeadObject, "./assets/man/head.png", vshader, fshader);
	Head->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	Head->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	Head->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(Head);//程序结束后释放数据

	//左大臂
	LeftUpperArm->setNormalize(true);
	LeftUpperArm->readObj("./assets/man/upper_arm.obj");

	painter3->bindObjectAndData(LeftUpperArm, LeftUpperArmObject, "./assets/man/upper_arm.png", vshader, fshader);
	LeftUpperArm->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	LeftUpperArm->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	LeftUpperArm->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(LeftUpperArm);//程序结束后释放数据
	

	//左小臂
	LeftLowerArm->setNormalize(true);
	LeftLowerArm->readObj("./assets/man/left_lower_arm.obj");

	painter3->bindObjectAndData(LeftLowerArm, LeftLowerArmObject, "./assets/man/left_lower_arm.png", vshader, fshader);
	LeftLowerArm->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	LeftLowerArm->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	LeftLowerArm->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(LeftLowerArm);//程序结束后释放数据



	//右大臂
	RightUpperArm->setNormalize(true);
	RightUpperArm->readObj("./assets/man/upper_arm.obj");

	painter3->bindObjectAndData(RightUpperArm, RightUpperArmObject, "./assets/man/upper_arm.png", vshader, fshader);
	RightUpperArm->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	RightUpperArm->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	RightUpperArm->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(RightUpperArm);//程序结束后释放数据


	//右小臂
	RightLowerArm->setNormalize(true);
	RightLowerArm->readObj("./assets/man/right_lower_arm.obj");

	painter3->bindObjectAndData(RightLowerArm, RightLowerArmObject, "./assets/man/right_lower_arm.png", vshader, fshader);
	RightLowerArm->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	RightLowerArm->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	RightLowerArm->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(RightLowerArm);//程序结束后释放数据

	//剑
	Sword->setNormalize(true);
	Sword->readObj("./assets/sword.obj");

	painter3->bindObjectAndData(Sword, SwordObject, "./assets/sword.png", vshader, fshader);
	Sword->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	Sword->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	Sword->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(Sword);//程序结束后释放数据


	//左大腿
	LeftUpperLeg->setNormalize(true);
	LeftUpperLeg->readObj("./assets/man/upper_leg.obj");

	painter3->bindObjectAndData(LeftUpperLeg, LeftUpperLegObject, "./assets/man/upper_leg.png", vshader, fshader);
	LeftUpperLeg->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	LeftUpperLeg->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	LeftUpperLeg->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(LeftUpperLeg);//程序结束后释放数据


	//左小腿
	LeftLowerLeg->setNormalize(true);
	LeftLowerLeg->readObj("./assets/man/lower_leg.obj");

	painter3->bindObjectAndData(LeftLowerLeg, LeftLowerLegObject, "./assets/man/lower_leg.png", vshader, fshader);
	LeftLowerLeg->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	LeftLowerLeg->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	LeftLowerLeg->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(LeftLowerLeg);//程序结束后释放数据



	//右大腿
	RightUpperLeg->setNormalize(true);
	RightUpperLeg->readObj("./assets/man/upper_leg.obj");

	painter3->bindObjectAndData(RightUpperLeg, RightUpperLegObject, "./assets/man/upper_leg.png", vshader, fshader);
	RightUpperLeg->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	RightUpperLeg->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	RightUpperLeg->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(RightUpperLeg);//程序结束后释放数据


	//右小腿
	RightLowerLeg->setNormalize(true);
	RightLowerLeg->readObj("./assets/man/lower_leg.obj");

	painter3->bindObjectAndData(RightLowerLeg, RightLowerLegObject, "./assets/man/lower_leg.png", vshader, fshader);
	RightLowerLeg->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	RightLowerLeg->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	RightLowerLeg->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	meshList.push_back(RightLowerLeg);//程序结束后释放数据






	glClearColor(0.5, 0.5, 0.5, 1.0);
	//glClearColor(0.0, 0.0, 0.0, 1.0);
}


int start = 0;

void autoRotate()
{
	start++;
	if (start - robot.changespeed >= 10) {
		r += 5;
		start = 0;
	}
}

void display()
{
// #ifdef __APPLE__ // 解决 macOS 10.15 显示画面缩小问题
// 	glViewport(0, 0, WIDTH * 2, HEIGHT * 2);
// #endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	camera->updateCamera();
	camera->viewMatrix = camera->getViewMatrix();
	camera->projMatrix = camera->getProjectionMatrix(false);

	// 设置物体的旋转位移
	mz->setTranslation(glm::vec3(-3, 0.0, 5));
	mz->setRotation(glm::vec3(0.0, 90.0, 0.0));
	mz->setScale(glm::vec3(s, s, s));
	autoRotate();
	fc->setTranslation(glm::vec3(-6, -0.02, -0.02));
	fc->setRotation(glm::vec3(0.0, r, 0.0));
	fc->setScale(glm::vec3(0.2, 0.2, 0.2));


	// 物体的变换矩阵
	glm::mat4 modelMatrix = glm::mat4(1.0);
	// 保持变换矩阵的栈
	MatrixStack mstack;
	Torso->setTranslation(glm::vec3(x_step_size, 0.38, z_step_size));

	// 躯干（这里我们希望机器人的躯干只绕Y轴旋转，所以只计算了RotateY）
	modelMatrix = glm::translate(modelMatrix, Torso->getTranslation());
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.Torso]), glm::vec3(0.0, 1.0, 0.0));
	torso(modelMatrix);

	mstack.push(modelMatrix); // 保存躯干变换矩阵
	// 头部（这里我们希望机器人的头部只绕Y轴旋转，所以只计算了RotateY）
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0, 0.1 * robot.TORSO_HEIGHT, -0.1));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.Head]), glm::vec3(0.0, 1.0, 0.0));
	head(modelMatrix);
	modelMatrix = mstack.pop(); // 恢复躯干变换矩阵


	// =========== 左臂 ===========
	mstack.push(modelMatrix);   // 保存躯干变换矩阵
	// 左大臂（这里我们希望机器人的左大臂只绕Z轴旋转，所以只计算了RotateZ，后面同理）
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5 * robot.UPPER_ARM_WIDTH, 0.1 * robot.TORSO_HEIGHT - 0.2, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.LeftUpperArm]), glm::vec3(0.0, 0.0, 1.0));
	left_upper_arm(modelMatrix);

	// @TODO: 左小臂
	mstack.push(modelMatrix);   // 保存躯干变换矩阵）
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5 * robot.UPPER_ARM_WIDTH + 0.5 * robot.LOWER_ARM_WIDTH + 0.3, 0.1 * robot.UPPER_ARM_HEIGHT - 0.2, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(-robot.theta[robot.LeftLowerArm]), glm::vec3(0.0, 1.0, 0.0));
	left_lower_arm(modelMatrix);
	modelMatrix = mstack.pop(); // 恢复躯干变换矩阵
	modelMatrix = mstack.pop();
	// =========== 右臂 ===========

	// @TODO: 右大臂
	mstack.push(modelMatrix);   // 保存躯干变换矩阵
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5 * robot.TORSO_WIDTH + 0.5 * robot.UPPER_ARM_WIDTH - 2.49, 0.1 * robot.TORSO_HEIGHT - 0.2, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.RightUpperArm]), glm::vec3(0.0, 0.0, 1.0));
	right_upper_arm(modelMatrix);

	// @TODO: 右小臂
	mstack.push(modelMatrix);   // 保存躯干变换矩阵
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5 * robot.UPPER_ARM_WIDTH - 0.5 * robot.LOWER_ARM_WIDTH - 0.3, 0.1 * robot.UPPER_ARM_HEIGHT - 0.2, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.RightLowerArm]), glm::vec3(0.0, 1.0, 0.0));
	right_lower_arm(modelMatrix);

	//剑
	mstack.push(modelMatrix);   // 保存躯干变换矩阵
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5 * robot.LOWER_ARM_WIDTH - 0.5 * robot.SWORD_WIDTH , 0.1 * robot.LOWER_ARM_HEIGHT - 0.19, 0.2));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.Sword]), glm::vec3(0.0, 1.0, 0.0));
	sword(modelMatrix);

	modelMatrix = mstack.pop(); // 恢复躯干变换矩阵
	modelMatrix = mstack.pop();
	modelMatrix = mstack.pop();

	// =========== 左腿 ===========

	// @TODO: 左大腿
	mstack.push(modelMatrix);   // 保存躯干变换矩阵）
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5 * robot.UPPER_LEG_WIDTH - 0.5 * robot.LOWER_LEG_WIDTH + 0.06, -robot.LOWER_LEG_WIDTH + 0.30, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.LeftUpperLeg]), glm::vec3(1.0, 0.0, 0.0));
	left_upper_leg(modelMatrix);

	// @TODO: 左小腿
	mstack.push(modelMatrix);   // 保存躯干变换矩阵）
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0, robot.LOWER_LEG_WIDTH - 0.7, 0.038));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.LeftLowerLeg]), glm::vec3(1.0, 0.0, 0.0));
	left_lower_leg(modelMatrix);
	modelMatrix = mstack.pop(); // 恢复躯干变换矩阵
	modelMatrix = mstack.pop();

	// =========== 右腿 ===========

	// @TODO: 右大腿
	mstack.push(modelMatrix);   // 保存躯干变换矩阵）
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5 * robot.UPPER_LEG_WIDTH - 0.5 * robot.LOWER_LEG_WIDTH - 0.06, -robot.LOWER_LEG_WIDTH + 0.30, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.RightUpperLeg]), glm::vec3(1.0, 0.0, 0.0));
	left_upper_leg(modelMatrix);

	// @TODO: 右小腿
	mstack.push(modelMatrix);   // 保存躯干变换矩阵）
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0, robot.LOWER_LEG_WIDTH - 0.7, 0.038));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.RightLowerLeg]), glm::vec3(1.0, 0.0, 0.0));
	right_lower_leg(modelMatrix);
	modelMatrix = mstack.pop(); // 恢复躯干变换矩阵
	modelMatrix = mstack.pop();




	
	painter3->drawMesh(Torso, object,light, camera);


	
	painter->drawMeshes(light, camera);
	painter2->drawMeshes2(light, camera);

	//glutSwapBuffers();
}


void printHelp()
{
	std::cout << "================================================" << std::endl;
	std::cout << "Use mouse / keyboard to controll the light position (drag)." << std::endl;
	std::cout << "================================================" << std::endl << std::endl;

	std::cout << "Keyboard Usage" << std::endl;
	std::cout <<
		"[Window]" << std::endl <<
		"ESC:		Exit" << std::endl <<
		"h:		    Print help message" << std::endl <<
		"a:         Man turn left" << std::endl <<
		"d:         Man turn right" << std::endl <<
		"f:         Speed up the rotation of the windmill" << std::endl <<


		"l/L:       Enlarge / Reduce sister" << std::endl <<
		"UP:        The man walks forward" << std::endl <<
		"DOWN:      The man walks back" << std::endl <<
		"LEFT:      The man walks left" << std::endl <<
		"RIGHT:     The man walks right" << std::endl << 




		std::endl <<
		"[Camera]" << std::endl <<
		"SPACE:		Reset camera parameters" << std::endl <<
		"u/U:		Increase/Decrease the rotate angle" << std::endl <<
		"i/I:		Increase/Decrease the up angle" << std::endl <<
		"o/O:		Increase/Decrease the camera radius" << std::endl << std::endl;

}
//控制机器人行走动作
void changestate()
{
	if (robot.action == 1)
	{
		robot.theta[0] = 0.0; // Torso
		robot.theta[1] = 0.0;    // Head
		robot.theta[2] = -10.0;    // RightUpperArm
		robot.theta[3] = 0.0;    // RightLowerArm
		robot.theta[4] = 10.0;    // LeftUpperArm
		robot.theta[5] = 0.0;   // LeftLowerArm
		robot.theta[6] = 20.0;    // RightUpperLeg
		robot.theta[7] = 0.0;    // RightLowerLeg
		robot.theta[8] = -20.0;    // LeftUpperLeg
		robot.theta[9] = 0.0;     // LeftLowerLeg
		robot.theta[10] = 60.0;    //sword
	}
	else if (robot.action == 2 || robot.action == 4)
	{
		robot.theta[0] = 0.0; // Torso
		robot.theta[1] = 0.0;    // Head
		robot.theta[2] = 0.0;    // RightUpperArm
		robot.theta[3] = 0.0;    // RightLowerArm
		robot.theta[4] = 0.0;    // LeftUpperArm
		robot.theta[5] = 0.0;   // LeftLowerArm
		robot.theta[6] = 0.0;    // RightUpperLeg
		robot.theta[7] = 0.0;    // RightLowerLeg
		robot.theta[8] = 0.0;    // LeftUpperLeg
		robot.theta[9] = 0.0;     // LeftLowerLeg
		robot.theta[10] = 60.0;    //sword
	}
	else if (robot.action == 3)
	{
		robot.theta[0] = 0.0;    // Torso
		robot.theta[1] = 0.0;    // Head
		robot.theta[2] = 10.0;    // RightUpperArm
		robot.theta[3] = 0.0;    // RightLowerArm
		robot.theta[4] = -10.0;    // LeftUpperArm
		robot.theta[5] = 0.0;   // LeftLowerArm
		robot.theta[6] = -20.0;    // RightUpperLeg
		robot.theta[7] = 0.0;    // RightLowerLeg
		robot.theta[8] = 20.0;    // LeftUpperLeg
		robot.theta[9] = 0.0;     // LeftLowerLeg
		robot.theta[10] = 60.0;    //sword
	}
}

//行走动作
void walk()
{
	robot.action = (robot.action++) % 4;
	changestate();
}




// 键盘响应函数
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	float tmp;
	glm::vec4 ambient;
	if (action == GLFW_PRESS||action ==GLFW_REPEAT) {
		switch (key)
		{
		case GLFW_KEY_ESCAPE: exit(EXIT_SUCCESS); break;
		case GLFW_KEY_H: printHelp(); break;
		case GLFW_KEY_1: Selected_mesh = robot.Torso; break;
		case GLFW_KEY_F: r+=25.0; break;//加速风车转
		case GLFW_KEY_L:
			if (mode == 0x0000)
			{
				s += 0.001; break;
			}
			else if (mode == GLFW_MOD_SHIFT)
			{
				s -= 0.001; break;
			}


		//向左转
		case GLFW_KEY_A:
			robot.theta[Selected_mesh] -= 10.0;
			if (robot.theta[Selected_mesh] > 360.0)
				robot.theta[Selected_mesh] -= 360.0;
			break;
        //向右转
 		case GLFW_KEY_D:
			robot.theta[Selected_mesh] += 10.0;
			if (robot.theta[Selected_mesh] < 0.0)
				robot.theta[Selected_mesh] += 360.0;
			break;

			//前后左右移动
		case GLFW_KEY_UP:
			walk();
			robot.theta[Selected_mesh] = 180.0;//转身
			z_step_size -= 0.1;
			//Torso->setTranslation(Torso->getTranslation() + glm::vec3(0.0, 0.0, -z_step_size));
			break;
		case GLFW_KEY_DOWN:
			walk();
			robot.theta[Selected_mesh] = 0.0;
			z_step_size += 0.1;
			//Torso->setTranslation(Torso->getTranslation() + glm::vec3(0.0, 0.0, z_step_size));

			break;
		case GLFW_KEY_LEFT:
			walk();
			robot.theta[Selected_mesh] = -90.0;
			x_step_size -= 0.1;
			//Torso->setTranslation(Torso->getTranslation() + glm::vec3(-x_step_size, 0.0, 0.0));

			break;
		case GLFW_KEY_RIGHT:
			walk();
			robot.theta[Selected_mesh] = 90.0;
			x_step_size += 0.1;
			//Torso->setTranslation(Torso->getTranslation() + glm::vec3(x_step_size, 0.0, 0.0));

			break;
		default:
			camera->keyboard(key, action, mode);
			break;
		}
	}
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	camera->mouse_callback(window, xpos, ypos);
}


void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera->scroll_callback(window, xoffset, yoffset);
}




void cleanData() {
	// 释放内存
	
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

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
	glfwWindowHint(GLFW_SAMPLES, 8);
	// 配置窗口属性
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "2019152115管迪扬期末大作业", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwSetScrollCallback(window, mouse_scroll_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	// 调用任何OpenGL的函数之前初始化GLAD
	// ---------------------------------------
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
		//reshape();

		// 交换颜色缓冲 以及 检查有没有触发什么事件（比如键盘输入、鼠标移动等）
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	cleanData();


	return 0;
}

// 每当窗口改变大小，GLFW会调用这个函数并填充相应的参数供你处理。
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}