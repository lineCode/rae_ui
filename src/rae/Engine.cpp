#include "rae/Engine.hpp"

#include <glm/glm.hpp>

#include "loguru/loguru.hpp"
#include "rae/core/ISystem.hpp"
#include "rae/visual/Mesh.hpp"
#include "rae/visual/Transform.hpp"
#include "rae/visual/Material.hpp"
#include "rae/core/Random.hpp"

using namespace rae;

//RAE_TODO: Split this into Engine & Viewport classes:
// Engine just handles all the systems and their updates.
// Viewport handles 3D picking etc... ?
Engine::Engine(GLFWwindow* window) :
	m_window(window),
	m_input(m_screenSystem),
	m_transformSystem(m_time),
	m_assetSystem(m_time, m_entitySystem),
	m_selectionSystem(m_transformSystem),
	m_cameraSystem(m_time, m_entitySystem, m_transformSystem, m_input),
	m_debugSystem(m_cameraSystem),
	m_rayTracer(m_time, m_cameraSystem),
	m_uiSystem(m_input, m_screenSystem, m_entitySystem, m_transformSystem, m_renderSystem, m_debugSystem),
	m_renderSystem(m_time, m_entitySystem, m_window, m_input, m_screenSystem,
		m_transformSystem, m_cameraSystem, m_assetSystem,
		m_selectionSystem, m_rayTracer),
	m_editorSystem(m_cameraSystem, m_renderSystem, m_assetSystem, m_selectionSystem, m_input, m_uiSystem)
{
	m_time.initTime(glfwGetTime());

	addSystem(m_input);
	addSystem(m_transformSystem);
	addSystem(m_cameraSystem);
	addSystem(m_assetSystem);
	addSystem(m_selectionSystem);
	addSystem(m_editorSystem);
	addSystem(m_uiSystem);
	addSystem(m_rayTracer);
	addSystem(m_renderSystem);

	addRenderer3D(m_renderSystem);
	addRenderer3D(m_editorSystem);
	addRenderer3D(m_debugSystem);
	addRenderer2D(m_renderSystem); // RAE_TODO should probably refactor and remove. Just infotext, that should be in uiSystem anyway.
	addRenderer2D(m_uiSystem);

	// RAE_TODO need to get rid of OpenGL picking hack button at id 0.
	Id emptyEntityId = m_entitySystem.createEntity(); // hack at index 0
	LOG_F(INFO, "Create empty hack entity at id: %i", emptyEntityId);

	// Load model
	Id meshID = m_assetSystem.createMesh("./data/models/bunny.obj");
	m_modelID = meshID;

	m_meshID			= m_renderSystem.createBox();
	m_materialID		= m_assetSystem.createMaterial(Color(0.2f, 0.5f, 0.7f, 0.0f));
	m_bunnyMaterialID	= m_assetSystem.createMaterial(Color(0.7f, 0.3f, 0.1f, 0.0f));
	m_buttonMaterialID	= m_assetSystem.createAnimatingMaterial(Color(0.0f, 0.0f, 0.1f, 0.0f));

	createTestWorld2();

	using std::placeholders::_1;
	m_input.connectMouseButtonPressEventHandler(std::bind(&Engine::onMouseEvent, this, _1));
	m_input.connectKeyEventHandler(std::bind(&Engine::onKeyEvent, this, _1));
}

void Engine::destroyEntity(Id id)
{
	m_destroyEntities.emplace_back(id);
}

void Engine::defragmentTablesAsync()
{
	m_defragmentTables = true;
}

void Engine::addSystem(ISystem& system)
{
	m_systems.push_back(&system);
}

void Engine::addRenderer3D(ISystem& system)
{
	m_renderers3D.push_back(&system);
}

void Engine::addRenderer2D(ISystem& system)
{
	m_renderers2D.push_back(&system);
}

void Engine::run()
{
	do {
		//glfwPollEvents(); // Don't use this here, it's for games. Use it in the inner loop if something is updating.
		// It will take up too much CPU all the time, even when nothing is happening.
		glfwWaitEvents(); //use this instead. It will sleep when no events are being received.

		while (m_running == true && update() == UpdateStatus::Changed)
		{
			// Swap buffers
			glfwSwapBuffers(m_window);

			glfwPollEvents();

			if (glfwWindowShouldClose(m_window) != 0)
				m_running = false;

			m_time.setPreviousTime();
		}

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(m_window, GLFW_KEY_ESCAPE) != GLFW_PRESS
		   && glfwWindowShouldClose(m_window) == 0);
}

UpdateStatus Engine::update()
{
	// Measure speed
	m_time.setTime(glfwGetTime());

	if (!m_destroyEntities.empty())
	{
		for (auto system : m_systems)
		{
			system->destroyEntities(m_destroyEntities);
		}
		m_entitySystem.destroyEntities(m_destroyEntities);
		m_destroyEntities.clear();
	}

	if (m_defragmentTables)
	{
		for (auto system : m_systems)
		{
			system->defragmentTables();
		}
		m_defragmentTables = false;
	}

	reactToInput(m_input);

	UpdateStatus engineUpdateStatus = UpdateStatus::NotChanged;

	//LOG_F(INFO, "FRAME START.");

	for (auto system : m_systems)
	{
		if (system->isEnabled())
		{
			UpdateStatus updateStatus = system->update();
			engineUpdateStatus = (updateStatus == UpdateStatus::Changed) ? UpdateStatus::Changed : engineUpdateStatus;
			//LOG_F(INFO, "%s update: %s", system->name(), bool(updateStatus == UpdateStatus::Changed) ? "true" : "false");
		}
	}

	m_renderSystem.beginFrame3D();
	for (auto system : m_renderers3D)
	{
		if (system->isEnabled())
		{
			system->render3D();
		}
	}
	m_renderSystem.endFrame3D();

	m_renderSystem.beginFrame2D();
	for (auto system : m_renderers2D)
	{
		if (system->isEnabled())
		{
			system->render2D(m_renderSystem.nanoVG());
		}
	}
	m_renderSystem.endFrame2D();

	//LOG_F(INFO, "FRAME END.");

	for (auto system : m_systems)
	{
		// A potential issue where isEnabled is changed to false earlier in the update,
		// and then onFrameEnd doesn't get called for the system.
		if (system->isEnabled())
		{
			system->onFrameEnd();
		}
	}

	return engineUpdateStatus;
}

void Engine::askForFrameUpdate()
{
	//glfwPostEmptyEvent(); //TODO need to update to GLFW 3.1
}

Id Engine::createAddObjectButton()
{
	Id id = m_entitySystem.createEntity();
	//LOG_F(INFO, "createAddObjectButton id: %i", id);
	m_transformSystem.addTransform(id, Transform(vec3(0.0f, 0.0f, 5.0f)));
	m_transformSystem.setPosition(id, vec3(0.0f, 0.0f, 0.0f));

	m_renderSystem.addMaterialLink(id, m_buttonMaterialID);
	m_renderSystem.addMeshLink(id, m_meshID);

	return id;
}

Id Engine::createRandomBunnyEntity()
{
	Id id = m_entitySystem.createEntity();
	LOG_F(INFO, "createRandomBunnyEntity id: %i", id);
	m_transformSystem.addTransform(id, Transform(vec3(getRandom(-10.0f, 10.0f), getRandom(-10.0f, 10.0f), getRandom(4.0f, 50.0f))));

	m_renderSystem.addMaterialLink(id, m_bunnyMaterialID);
	m_renderSystem.addMeshLink(id, m_modelID);

	return id;
}

Id Engine::createRandomCubeEntity()
{
	Id id = m_entitySystem.createEntity();
	LOG_F(INFO, "createRandomCubeEntity id: %i", id);
	m_transformSystem.addTransform(id, Transform(vec3(getRandom(-10.0f, 10.0f), getRandom(-10.0f, 10.0f), getRandom(4.0f, 50.0f))));

	m_renderSystem.addMaterialLink(id, m_materialID);
	m_renderSystem.addMeshLink(id, m_meshID);

	return id;
}

Id Engine::createCube(const vec3& position, const Color& color)
{
	Id id = m_entitySystem.createEntity();
	//LOG_F(INFO, "createCube id: %i", id);
	// The desired API:
	m_transformSystem.addTransform(id, Transform(position));
	//m_geometrySystem.setMesh(entity, m_meshID);
	//m_materialSystem.setMaterial(entity, color);

	m_assetSystem.addMaterial(id, Material(color));
	m_renderSystem.addMeshLink(id, m_meshID);

	return id;
}

Id Engine::createBunny(const vec3& position, const Color& color)
{
	Id id = m_entitySystem.createEntity();
	//LOG_F(INFO, "createBunny id: %i", id);
	m_transformSystem.addTransform(id, Transform(position));

	m_renderSystem.addMaterialLink(id, m_bunnyMaterialID);
	m_renderSystem.addMeshLink(id, m_modelID);

	return id;
}

void Engine::createTestWorld()
{
	createAddObjectButton(); // at index 1
}

void Engine::createTestWorld2()
{
	//LOG_F(INFO, "createTestWorld2");

	//createAddObjectButton(); // at index 1

	auto cube0 = createCube(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.8f, 0.3f, 0.3f, 0.0f));
	
	auto cube1 = createCube(glm::vec3(1.0f, 0.0f, -1.0f), glm::vec4(0.8f, 0.6f, 0.2f, 0.0f));
	auto cube2 = createCube(glm::vec3(-0.5f, 0.65f, -1.0f), glm::vec4(0.8f, 0.4f, 0.8f, 0.0f));
	auto cube3 = createCube(glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec4(0.8f, 0.5f, 0.3f, 0.0f));
	auto cube4 = createCube(glm::vec3(-3.15f, 0.1f, -5.0f), glm::vec4(0.05f, 0.2f, 0.8f, 0.0f));

	auto bunny1 = createBunny(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(0.05f, 0.2f, 0.8f, 0.0f));

	/*Hierarchy& hierarchy1 = m_entitySystem.createHierarchy();
	cube1.addComponent( (int)ComponentType::HIERARCHY, hierarchy1.id() );
	hierarchy1.addChild(cube2.id());

	Hierarchy& hierarchy2 = m_entitySystem.createHierarchy();
	cube2.addComponent( (int)ComponentType::HIERARCHY, hierarchy2.id() );
	hierarchy2.setParent(cube1.id());
	hierarchy2.addChild(cube3.id());

	Hierarchy& hierarchy3 = m_entitySystem.createHierarchy();
	cube3.addComponent( (int)ComponentType::HIERARCHY, hierarchy3.id() );
	hierarchy3.setParent(cube2.id());
	*/
}

void Engine::osEventResizeWindow(int width, int height)
{
	// TODO could pass it straight to screenSystem and cameraSystem.
	m_renderSystem.osEventResizeWindow(width, height);
}

void Engine::osEventResizeWindowPixels(int width, int height)
{
	// TODO could pass it straight to screenSystem and cameraSystem.
	m_renderSystem.osEventResizeWindowPixels(width, height);
}

void Engine::osMouseButtonPress(int set_button, float set_xP, float set_yP)
{
	const auto& window = m_screenSystem.window();
	// Have to scale input on retina screens:
	set_xP = set_xP * window.screenPixelRatio();
	set_yP = set_yP * window.screenPixelRatio();

	//LOG_F(INFO, "osMouseButtonPress after screenPixelRatio: %f x: %f y: %f", window.screenPixelRatio(),
	//	set_xP, set_yP);

	float setAmount = 0.0f;
	m_input.osMouseEvent(
		EventType::MouseButtonPress,
		set_button,
		set_xP,
		set_yP,
		setAmount);
}

void Engine::osMouseButtonRelease(int set_button, float set_xP, float set_yP)
{
	const auto& window = m_screenSystem.window();
	// Have to scale input on retina screens:
	set_xP = set_xP * window.screenPixelRatio();
	set_yP = set_yP * window.screenPixelRatio();

	float setAmount = 0.0f;
	m_input.osMouseEvent(
		EventType::MouseButtonRelease,
		set_button,
		set_xP,
		set_yP,
		setAmount);
}

void Engine::osMouseMotion(float set_xP, float set_yP)
{
	const auto& window = m_screenSystem.window();
	// Have to scale input on retina screens:
	set_xP = set_xP * window.screenPixelRatio();
	set_yP = set_yP * window.screenPixelRatio();

	float setAmount = 0.0f;
	m_input.osMouseEvent(
		EventType::MouseMotion,
		(int)MouseButton::Undefined,
		set_xP,
		set_yP,
		setAmount);
}

void Engine::osScrollEvent(float scrollX, float scrollY)
{
	m_input.osScrollEvent(scrollX, scrollY);
}

void Engine::osKeyEvent(int key, int scancode, int action, int mods)
{
	// glfw mods are not handled at the moment
	EventType eventType = EventType::Undefined;
	if (action == GLFW_PRESS)
		eventType = EventType::KeyPress;
	else if (action == GLFW_RELEASE)
		eventType = EventType::KeyRelease;

	m_input.osKeyEvent(eventType, key, (int32_t)scancode);
}

void Engine::onMouseEvent(const Input& input)
{
	if (input.eventType == EventType::MouseButtonPress)
	{
		if (input.mouse.eventButton == MouseButton::First)
		{
			const auto& window = m_screenSystem.window();

			//LOG_F(INFO, "mouse press: x: %f y: %f", input.mouse.x, input.mouse.y);
			//LOG_F(INFO, "mouse press: xP: %f yP: %f", (int)m_screenSystem.heightToPixels(input.mouse.x) + (m_renderSystem.windowPixelWidth() / 2),
			//	m_renderSystem.windowPixelHeight() - (int)m_screenSystem.heightToPixels(input.mouse.y) - (m_renderSystem.windowPixelHeight() / 2));

			unsigned char res[4];

			m_renderSystem.renderPicking();

			//glGetIntegerv(GL_VIEWPORT, viewport);
			glReadPixels(
				(int)m_screenSystem.heightToPixels(input.mouse.x) + (window.pixelWidth() / 2),
				window.pixelHeight() - (int)m_screenSystem.heightToPixels(input.mouse.y) - (window.pixelHeight() / 2),
				1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &res);

			// Decode entity ID from red and green channels!
			int pickedID = res[0] + (res[1] * 256);

			//m_renderSystem.m_pickedString = std::to_string(pickedID) + " is " + std::to_string(res[0]) + " and " + std::to_string(res[1]);

			if (pickedID == 0)
			{
				// Hit the background.
				m_selectionSystem.clearPixelClicked();
			}
			else if (pickedID == 13)
			{
				createRandomCubeEntity();
				createRandomBunnyEntity();
			}
			else
			{
				//LOG_F(INFO, "Pixel clicked entity: %i", pickedID);
				m_selectionSystem.setPixelClicked({ pickedID });
			}
		}
	}
}

void Engine::onKeyEvent(const Input& input)
{
	if (input.eventType == EventType::KeyPress)
	{
		switch (input.key.value)
		{
			case KeySym::Escape: m_running = false; break;
			case KeySym::R: m_renderSystem.clearImageRenderer(); break;
			case KeySym::G:
				m_renderSystem.toggleGlRenderer(); // more like debug view currently
				m_rayTracer.toggleIsEnabled();
				break;
			case KeySym::Tab: m_rayTracer.toggleInfoText(); break;
			case KeySym::Y: m_rayTracer.toggleBufferQuality(); break;
			case KeySym::U: m_rayTracer.toggleFastMode(); break;
			case KeySym::H: m_rayTracer.toggleVisualizeFocusDistance(); break;
			case KeySym::_1: m_rayTracer.showScene(1); break;
			case KeySym::_2: m_rayTracer.showScene(2); break;
			case KeySym::_3: m_rayTracer.showScene(3); break;
			default:
			break;
		}
	}
}

void Engine::reactToInput(const Input& input)
{
	if (input.getKeyState(KeySym::I))
	{
		createRandomCubeEntity();
		createRandomBunnyEntity();
	}

	if (input.getKeyState(KeySym::O))
	{
		LOG_F(INFO, "Destroy biggestId: %i", m_entitySystem.biggestId());
		destroyEntity((Id)getRandomInt(20, m_entitySystem.biggestId()));
	}

	if (input.getKeyState(KeySym::P))
	{
		defragmentTablesAsync();
	}

	// TODO use KeySym::Page_Up
	if (input.getKeyState(KeySym::K)) { m_rayTracer.minusBounces(); }
	if (input.getKeyState(KeySym::L)) { m_rayTracer.plusBounces(); }
}
