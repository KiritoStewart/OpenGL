#ifndef _MESH_PAINTER_H_
#define _MESH_PAINTER_H_

#include "TriMesh.h"
#include "Angel.h"

#include "Camera.h"

#include <vector>
#include <map>


class MeshPainter
{

public:
    MeshPainter();
    ~MeshPainter();

    int getMeshIndex(std::string name);

    std::vector<TriMesh *>& getMeshes();

	// 读取纹理文件
    void load_texture_STBImage(const std::string &file_name, GLuint& texture);

	// 传递光线材质数据的
    // void bindLightAndMaterial( int mesh_id, int light_id, Camera* camera );
    void bindLightAndMaterial(TriMesh* mesh, openGLObject& object, Light* light, Camera* camera);

    void bindObjectAndData(TriMesh *mesh, openGLObject &object, const std::string &texture_image, const std::string &vshader, const std::string &fshader);

	// 添加物体
    void addMesh( TriMesh* mesh, const std::string &name, const std::string &texture_image, const std::string &vshader, const std::string &fshader );

	// 绘制物体
    void drawMesh(TriMesh* mesh, openGLObject &object, Light *light, Camera* camera, bool isShadow, glm::mat4 fatherMatrix = glm::mat4(1.0f));

    void drawSkybox(TriMesh* mesh, openGLObject& object, Light* light, Camera* camera);

    // 绘制多个物体
    void drawMeshes(Light *light, Camera* camera);

	// 清空数据
    void cleanMeshes();

private:
    std::vector<TriMesh *> meshes;
    std::map<std::string, int> meshes_name;

};

#endif