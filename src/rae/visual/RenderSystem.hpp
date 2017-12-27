#ifndef RAE_RENDERSYSTEM_HPP
#define RAE_RENDERSYSTEM_HPP

#include <string>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include "rae/core/Types.hpp"
#include "rae/core/ISystem.hpp"
#include "TransformSystem.hpp"
#include "RayTracer.hpp"
#include "rae/ui/UISystem.hpp"

#include "rae/image/ImageBuffer.hpp" //JONDE TEMP HACK

struct NVGcontext;

namespace rae
{

class CameraSystem;
class TransformSystem;
class ObjectFactory;
class Transform;
class Material;
class Mesh;
class Entity;
class Input;

class RenderSystem : public ISystem
{
public:
	RenderSystem(ObjectFactory& objectFactory,
		GLFWwindow* setWindow,
		Input& input,
		CameraSystem& cameraSystem,
		TransformSystem& transformSystem,
		UISystem& uiSystem,
		RayTracer& rayTracer);
	~RenderSystem();

	String name() override { return "RenderSystem"; }

	void initNanoVG();
	void init();

	void checkErrors(const char *file, int line);

	Id createBox();
	Id createMesh(const String& filename);
	Id createMaterial(const Colour& color);
	Id createAnimatingMaterial(const Colour& color);

	bool update(double time, double delta_time) override;

	void render(double time, double delta_time);
	void renderPicking();
	void render2dBackground(double time, double delta_time);
	void render2d(double time, double delta_time);

	//JONDE TEMP:
	void renderImageBuffer(NVGcontext* vg, ImageBuffer& readBuffer,
		float x, float y, float w, float h);
	ImageBuffer& getBackgroundImage() { return m_backgroundImage; }

	void renderMesh(const Transform& transform, const Material& material, const Mesh& mesh);
	void renderMeshPicking(const Transform& transform, const Mesh& mesh, Id id);

	void osEventResizeWindow(int width, int height);
	void osEventResizeWindowPixels(int width, int height);

	int   windowPixelHeight() { return m_windowPixelHeight; }
	int   windowPixelWidth()  { return m_windowPixelWidth;  }
	int   windowHeight()      { return m_windowHeight;      }
	int   windowWidth()       { return m_windowWidth;       }
	float screenPixelRatio()  { return m_screenPixelRatio;  }

	// Temp before we get keyboard Input class
	void clearImageRenderer();
	void toggleGlRenderer()
	{
		m_glRendererOn = !m_glRendererOn;
	}

	void addMesh(Id id, Mesh&& comp);
	const Mesh& getMesh(Id id);
	void addMeshLink(Id id, Id linkId);

	void addMaterial(Id id, Material&& comp);
	const Material& getMaterial(Id id);
	void addMaterialLink(Id id, Id linkId);

	int meshCount() { return m_meshes.size(); }
	int materialCount() { return m_materials.size(); }

protected:

	// basic shader

	GLuint shaderID;

	GLuint modelViewMatrixUni;
	GLuint viewMatrixUni;
	GLuint modelMatrixUni;
	GLuint lightPositionUni;
	GLuint textureUni;

	// picking shader

	GLuint pickingShaderID;
	GLuint pickingModelViewMatrixUni;
	GLuint entityUni;
//public:	std::string m_pickedString;

protected:
	GLFWwindow* m_window;

	// nanovg context
	NVGcontext* vg;
	
	// dependencies
	ObjectFactory& m_objectFactory;
	Input& m_input;
	UISystem& m_uiSystem;

	int m_windowWidth;
	int m_windowHeight;
	int m_windowPixelWidth;
	int m_windowPixelHeight;
	float m_screenPixelRatio;

	Transform* debugTransform = nullptr;
	Transform* debugTransform2 = nullptr;

	int m_nroFrames;
	double m_fpsTimer;
	std::string m_fpsString;

	bool m_glRendererOn = false;

	CameraSystem&			m_cameraSystem;
	TransformSystem&		m_transformSystem;
	RayTracer&				m_rayTracer;

	ImageBuffer m_backgroundImage;

	// Move to VisualSystem or something?:
	Table<Mesh>			m_meshes;
	Table<Id>			m_meshLinks;
	Table<Material>		m_materials;
	Table<Id>			m_materialLinks;
};

}

#endif
