#pragma once

#include <array>

#include "rae/core/ISystem.hpp"
#include "rae/entity/Table.hpp"

#include "rae/visual/Mesh.hpp"
#include "rae/visual/Material.hpp"

#include "rae/ui/DebugSystem.hpp"

namespace rae
{

class Camera;
class CameraSystem;
class RenderSystem;
class AssetSystem;
class SelectionSystem;
class Input;
class UISystem;

enum class Axis
{
	X,
	Y,
	Z,
	Count
};

enum class HandleStatus
{
	NotHandled,
	Handled
};

class IGizmo
{
public:
	const vec3& position() const { return m_position; }
	void setPosition(const vec3& position) { m_position = position; }

	bool isVisible() const { return m_visible; }
	void show() { m_visible = true; }
	void hide() { m_visible = false; }

	bool isHovered() const
	{
		for (int i = 0; i < (int)Axis::Count; ++i)
		{
			if (m_axisHovers[i])
				return true;
		}
		return false;
	}

	bool isActive() const
	{
		for (int i = 0; i < (int)Axis::Count; ++i)
		{
			if (m_axisActives[i])
				return true;
		}
		return false;
	}

	Axis getActiveAxis() const
	{
		for (int i = 0; i < (int)Axis::Count; ++i)
		{
			if (m_axisActives[i])
				return (Axis)i;
		}
	}

	void activateHovered()
	{
		for (int i = 0; i < (int)Axis::Count; ++i)
		{
			m_axisActives[i] = m_axisHovers[i];
		}
	}

	void deactivate()
	{
		for (int i = 0; i < (int)Axis::Count; ++i)
		{
			m_axisActives[i] = false;
		}
	}

protected:
	bool m_visible = false;

	vec3 m_position;

	std::array<Transform,	(int)Axis::Count>	m_axisTransforms;
	std::array<bool,		(int)Axis::Count>	m_axisHovers = { false, false, false };
	std::array<bool,		(int)Axis::Count>	m_axisActives = { false, false, false };
};

struct LineHandle
{
	int axisIndex;
	vec3 tipPosition;
	float distanceFromCamera;
};

class LineGizmo : public IGizmo
{
public:
	LineGizmo();

	bool hover(const Ray& mouseRay, const Camera& camera);
	void render3D(const Camera& camera, RenderSystem& renderSystem, AssetSystem& assetSystem);

	void setGizmoMaterialId(Id id);

	vec3 getActiveAxisVector() const;
	vec3 activeAxisDelta(const Camera& camera, const Ray& mouseRay, const Ray& previousMouseRay);// const;

	//debug:
	Line m_debugIntersectionLine;
	Line m_debugLine;

protected:

	std::array<LineHandle, (int)Axis::Count> m_sortedLineHandles;

	float m_gizmoSizeMultiplier = 0.075f;
	float m_hoverMarginMultiplier = 1.4f;
	float m_hoverThicknessMultiplier = 3.0f;
	float m_coneLengthMultiplier = 4.0f;
	Mesh m_lineMesh;
	Mesh m_coneMesh;
	Id m_materialId;
};

class TranslateGizmo : public LineGizmo
{
public:
	TranslateGizmo(){}
};

class TransformTool
{
public:
	HandleStatus handleInput(Input& input, const Camera& camera, SelectionSystem& selectionSystem);
	bool hover(const Ray& mouseRay, const Camera& camera);
	void render3D(const Camera& camera, RenderSystem& renderSystem, AssetSystem& assetSystem);

	void setGizmoMaterialId(Id id);

	void onSelectionChanged(SelectionSystem& selectionSystem);

	void translateSelected(const vec3& delta, SelectionSystem& selectionSystem);

protected:
	TranslateGizmo m_translateGizmo;

	Ray m_mouseRay;
	Ray m_previousMouseRay;
};

class EditorSystem : public ISystem
{
public:
	EditorSystem(CameraSystem& cameraSystem, RenderSystem& renderSystem, AssetSystem& assetSystem,
		SelectionSystem& selectionSystem, Input& input, UISystem& uiSystem);

	UpdateStatus update() override;
	void render3D() override;
protected:
	CameraSystem&		m_cameraSystem;
	RenderSystem&		m_renderSystem;
	AssetSystem&		m_assetSystem;
	SelectionSystem&	m_selectionSystem;
	Input&				m_input;
	UISystem&			m_uiSystem;

	TransformTool		m_transformTool;
};

}
