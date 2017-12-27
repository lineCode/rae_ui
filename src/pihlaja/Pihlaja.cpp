#include "pihlaja/Pihlaja.hpp"

Pihlaja::Pihlaja(GLFWwindow* glfwWindow) :
	m_engine(glfwWindow),
	m_avSystem(m_engine.getRenderSystem()),
	m_screenImage(m_engine.getRenderSystem().getBackgroundImage()),
	m_input(m_engine.getInput())
{
	m_engine.addSystem(m_avSystem);
	m_engine.addSystem(*this);

	using std::placeholders::_1;
	m_input.connectKeyEventHandler(std::bind(&Pihlaja::onKeyEvent, this, _1));

	//m_videoAssetId = m_avSystem.loadAsset("/Users/joonaz/Documents/jonas/hdr_testi_matskut2017/MVI_9132.MOV");
	//m_videoAssetId = m_avSystem.loadAsset("/Users/joonaz/Documents/jonas/hdr_testi_matskut2017/glass/MVI_8882.MOV");
	m_videoAssetId = m_avSystem.loadAsset("/Users/joonaz/Documents/jonas/hdr_testi_matskut2017/test5.mov");

	////////m_hdrFlow.setExposureWeight(0.75f);

	initUI();
}

void Pihlaja::initUI()
{
	auto& uiSystem = m_engine.getUISystem();

	m_playButtonId = uiSystem.createButton("Play",
		virxels(0.0f, 350.0f, 0.0f),
		virxels(98.0f, 25.0f, 0.1f),
		std::bind(&Pihlaja::togglePlay, this));
	uiSystem.setActive(m_playButtonId, m_play);
	//TODO IDEA: uiSystem.bindActive(m_playButtonId, std::bind(&Pihlaja::isPlay, this));

	uiSystem.createButton("Rewind",
		virxels(-100.0f, 350.0f, 0.0f),
		virxels(98.0f, 25.0f, 0.1f),
		std::bind(&Pihlaja::rewind, this));

	m_debugNeedsFrameUpdateButtonId = uiSystem.createTextBox("NeedsFrameUpdate",
		virxels(-100.0f, 380.0f, 0.0f),
		virxels(98.0f, 25.0f, 0.1f));
	uiSystem.setActive(m_debugNeedsFrameUpdateButtonId, m_needsFrameUpdate);
}

void Pihlaja::togglePlay()
{
	m_play = !m_play;

	// TODO would be nice if all UI state could be defined in one place, and not sprinkled all around other code.
	auto& uiSystem = m_engine.getUISystem();
	uiSystem.setActive(m_playButtonId, m_play);
}

void Pihlaja::rewind()
{
	if (not m_avSystem.hasAsset(m_videoAssetId))
	{
		std::cout << "No asset found in AVSystem.\n";
		return;
	}

	auto& asset = m_avSystem.getAsset(m_videoAssetId);
	if (not asset.isLoaded())
	{
		std::cout << "Asset is not loaded.\n";
		return;
	}

	asset.seekToStart();
	setNeedsFrameUpdate(true);
	m_frameCount = 0;
}

void Pihlaja::setNeedsFrameUpdate(bool value)
{
	m_needsFrameUpdate = value;
	auto& uiSystem = m_engine.getUISystem();
	uiSystem.setActive(m_debugNeedsFrameUpdateButtonId, m_needsFrameUpdate);

	m_engine.askForFrameUpdate();
}

void Pihlaja::onKeyEvent(const Input& input)
{
	if (input.eventType == EventType::KeyPress)
	{
		switch (input.key.value)
		{
			case KeySym::space:
				togglePlay();
				break;
			case KeySym::Home:
				rewind();
			break;
			case KeySym::_1:
				m_evenFrames = true;
				break;
			case KeySym::_2:
				m_evenFrames = false;
				break;
			case KeySym::R:
				if (m_videoRenderingState == VideoRenderingState::Player)
					m_videoRenderingState = VideoRenderingState::RenderToScreen;
				else m_videoRenderingState = VideoRenderingState::Player;
				setNeedsFrameUpdate(true);
				break;
			case KeySym::E:
				if (m_videoRenderingState == VideoRenderingState::Player)
					m_videoRenderingState = VideoRenderingState::RenderToDisk;
				else m_videoRenderingState = VideoRenderingState::Player;
				setNeedsFrameUpdate(true);
				break;
			default:
			break;
		}
	}
}

void Pihlaja::run()
{
	m_engine.run();
}

// OpticalFlow version
bool Pihlaja::update(double time, double deltaTime, std::vector<Entity>&)
{
	if (not m_play and not m_needsFrameUpdate)
	{
		return false;
	}

	if (m_opticalFlow.getState() == EffectNodeState::Nothing ||
		m_opticalFlow.getState() == EffectNodeState::WaitingForData)
	{
		if (not m_avSystem.hasAsset(m_videoAssetId))
		{
			std::cout << "No asset found in AVSystem.\n";
			return false;
		}

		auto& asset = m_avSystem.getAsset(m_videoAssetId);
		if (not asset.isLoaded())
		{
			std::cout << "Asset is not loaded.\n";
			return false;
		}

		AVFrame* frameRGB = asset.pullFrame();
		m_frameCount++;

		if (m_videoRenderingState == VideoRenderingState::Player)
		{
			m_avSystem.copyFrameToImage(frameRGB, m_screenImage);
			setNeedsFrameUpdate(false);
		}
		else if (m_videoRenderingState == VideoRenderingState::RenderToScreen or
				 m_videoRenderingState == VideoRenderingState::RenderToDisk)
		{
			m_opticalFlow.pushFrame(frameRGB);
		}

		//if (m_frameCount % 2 == 0)
		//{
		//	m_opticalFlow.pushFrame(frameRGB);
		//}
	}
	else if (m_opticalFlow.getState() == EffectNodeState::Processing)
	{
		if (m_videoRenderingState == VideoRenderingState::RenderToScreen or
			m_videoRenderingState == VideoRenderingState::RenderToDisk)
		{
			//m_opticalFlow.update(time, deltaTime, m_screenImage);
			m_opticalFlow.update(time, deltaTime);
		}
	}
	else if (m_opticalFlow.getState() == EffectNodeState::Done)
	{
		m_opticalFlow.update(time, deltaTime);
		if (m_videoRenderingState == VideoRenderingState::RenderToScreen)
		{
			m_opticalFlow.writeFrameToImage(m_screenImage);
		}
		else if (m_videoRenderingState == VideoRenderingState::RenderToDisk)
		{
			m_opticalFlow.writeFrameToDiskAndImage("/Users/joonaz/Documents/jonas/hdr_testi_matskut2017/glassrender/",
				m_screenImage);
		}
		///////////NOT: m_opticalFlow.waitForData();
		
		//m_opticalFlow.copyMatToImage(m_opticalFlow.getoutput, m_screenImage);
	}

	//if (not m_opticalFlow.isDone())
	//{
	//	m_opticalFlow.process();
	//}

	//m_opticalFlow.update(time, deltaTime, m_screenImage);
	
	return m_needsFrameUpdate || m_play;
	//return false;
}

/* HdrFlow version:
bool Pihlaja::update(double time, double deltaTime, std::vector<Entity>&)
{
	if (not m_play and not m_needsFrameUpdate)
		return false;

	if (m_hdrFlow.getState() == EffectNodeState::Nothing ||
		m_hdrFlow.getState() == EffectNodeState::WaitingForData)
	{
		if (not m_avSystem.hasAsset(m_videoAssetId))
		{
			std::cout << "No asset found in AVSystem.\n";
			return false;
		}

		auto& asset = m_avSystem.getAsset(m_videoAssetId);
		if (not asset.isLoaded())
		{
			std::cout << "Asset is not loaded.\n";
			return false;
		}

		AVFrame* frameRGB = asset.pullFrame();
		m_frameCount++;

		if (m_videoRenderingState == VideoRenderingState::Player)
		{
			if (m_needsFrameUpdate or (m_evenFrames and m_frameCount % 2 == 0))
				m_avSystem.copyFrameToImage(frameRGB, m_screenImage);
			else if (m_needsFrameUpdate or (not m_evenFrames and m_frameCount % 2))
				m_avSystem.copyFrameToImage(frameRGB, m_screenImage);
			m_needsFrameUpdate = false;
		}
		else if (m_videoRenderingState == VideoRenderingState::RenderToScreen or
				 m_videoRenderingState == VideoRenderingState::RenderToDisk)
		{
			m_hdrFlow.pushFrame(frameRGB);
		}

		//if (m_frameCount % 2 == 0)
		//{
		//	m_opticalFlow.pushFrame(frameRGB);
		//}
	}
	else if (m_hdrFlow.getState() == EffectNodeState::Processing)
	{
		if (m_videoRenderingState == VideoRenderingState::RenderToScreen or
			m_videoRenderingState == VideoRenderingState::RenderToDisk)
		{
			//m_opticalFlow.update(time, deltaTime, m_screenImage);
			m_hdrFlow.update(time, deltaTime);
		}
	}
	else if (m_hdrFlow.getState() == EffectNodeState::Done)
	{
		if (m_videoRenderingState == VideoRenderingState::RenderToScreen)
		{
			m_hdrFlow.writeFrameToImage(m_screenImage);
		}
		else if (m_videoRenderingState == VideoRenderingState::RenderToDisk)
		{
			m_hdrFlow.writeFrameToDiskAndImage("/Users/joonaz/Documents/jonas/hdr_testi_matskut2017/glassrender/",
				m_screenImage);
		}
		m_hdrFlow.waitForData();
	}

	//if (not m_opticalFlow.isDone())
	//{
	//	m_opticalFlow.process();
	//}

	//m_opticalFlow.update(time, deltaTime, m_screenImage);
	
	return m_needsFrameUpdate || m_play;
	//return false;
}
*/
