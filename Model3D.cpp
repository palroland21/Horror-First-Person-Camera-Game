#include "Model3D.hpp"

namespace gps {

    void Model3D::LoadModel(std::string fileName) {
        std::string basePath = fileName.substr(0, fileName.find_last_of('/')) + "/";
        ReadOBJ(fileName, basePath);
    }

    void Model3D::LoadModel(std::string fileName, std::string basePath) {
        ReadOBJ(fileName, basePath);
    }

    void Model3D::Draw(gps::Shader shaderProgram) {
        for (int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shaderProgram);
    }

    void Model3D::ReadOBJ(std::string fileName, std::string basePath) {

        std::cout << "Loading : " << fileName << std::endl;

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string err;
        bool ret = tinyobj::LoadObj(
            &attrib, &shapes, &materials, &err,
            fileName.c_str(), basePath.c_str(), true
        );

        if (!err.empty()) std::cerr << err << std::endl;
        if (!ret) exit(1);

        std::cout << "# of shapes    : " << shapes.size() << std::endl;
        std::cout << "# of materials : " << materials.size() << std::endl;

        for (size_t s = 0; s < shapes.size(); s++) {

            // Cum era inainte -> aplica un singur material pe tot shape-ul, chiar daca OBJ avea materiale diferite pe fiecare fata
            // Acuma: Mesh separat pt fiecare materialID
            std::map<int, std::vector<gps::Vertex>> materialVertices;
            std::map<int, std::vector<GLuint>> materialIndices;

            size_t index_offset = 0;

            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

                int fv = shapes[s].mesh.num_face_vertices[f];
                int materialId = shapes[s].mesh.material_ids[f];
                if (materialId < 0) materialId = 0;

                for (size_t v = 0; v < fv; v++) {

                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
                    if (idx.vertex_index >= 0) {
                        vx = attrib.vertices[3 * idx.vertex_index + 0];
                        vy = attrib.vertices[3 * idx.vertex_index + 1];
                        vz = attrib.vertices[3 * idx.vertex_index + 2];
                    }

                    float nx = 0.0f, ny = 1.0f, nz = 0.0f;
                    if (idx.normal_index >= 0 && !attrib.normals.empty()) {
                        nx = attrib.normals[3 * idx.normal_index + 0];
                        ny = attrib.normals[3 * idx.normal_index + 1];
                        nz = attrib.normals[3 * idx.normal_index + 2];
                    }

                    float tx = 0.5f, ty = 0.5f;
                    if (idx.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                        tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                        ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                    }

                    gps::Vertex vert;
                    vert.Position = glm::vec3(vx, vy, vz);
                    vert.Normal = glm::vec3(nx, ny, nz);
                    vert.TexCoords = glm::vec2(tx, ty);

                    materialVertices[materialId].push_back(vert);
                   /* materialIndices[materialId].push_back(
                        static_cast<GLuint>(materialVertices[materialId].size() - 1)
                    );*/
                    materialIndices[materialId].push_back((GLuint)(materialVertices[materialId].size() - 1));

                }

                index_offset += fv;
            }

            for (auto& kv : materialVertices) {

                // kv.first = materialID
                // kv.second = vectorul de vertecsi pt acel material

                int matID = kv.first;
                std::vector<gps::Texture> textures;

                if (matID >= 0 && matID < materials.size()) { // verific daca e valid => altfel da crash

                    // incarc textura difuza din .mtl (leg textura difuza cu "diffuseTexture" din shader)
                    std::string diffusePath = materials[matID].diffuse_texname;
                    if (!diffusePath.empty()) {
                        textures.push_back(LoadTexture(basePath + diffusePath, "diffuseTexture"));
                    }

                    // incarc textura speculara (din .mtl)
                    std::string specularPath = materials[matID].specular_texname;
                    if (!specularPath.empty()) {
                        textures.push_back(LoadTexture(basePath + specularPath, "specularTexture"));
                    }
                }

                // construiesc un Mesh separat pt fiecare material si il salvez in mashes
                meshes.push_back(
                    gps::Mesh(kv.second, materialIndices[matID], textures)
                );
            }
        }
    }

    gps::Texture Model3D::LoadTexture(std::string path, std::string type) {

        for (int i = 0; i < loadedTextures.size(); i++) {
            if (loadedTextures[i].path == path)
                return loadedTextures[i];
        }

        gps::Texture tex;
        tex.id = ReadTextureFromFile(path.c_str());
        tex.type = type;
        tex.path = path;

        loadedTextures.push_back(tex);
        return tex;
    }

    GLuint Model3D::ReadTextureFromFile(const char* file_name) {

        int x, y, n;
        unsigned char* data = stbi_load(file_name, &x, &y, &n, 4);

        if (!data) {
            std::cerr << "ERROR loading texture: " << file_name << std::endl;
            return 0;
        }

        GLuint texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);

        return texID;
    }

    Model3D::~Model3D() {

        for (auto& t : loadedTextures)
            glDeleteTextures(1, &t.id);

        for (auto& m : meshes) {
            GLuint VBO = m.getBuffers().VBO;
            GLuint EBO = m.getBuffers().EBO;
            GLuint VAO = m.getBuffers().VAO;
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteVertexArrays(1, &VAO);
        }
    }
}
