#include "engine.hpp"

using namespace Ona;

int main(int argv, char const * const * argc) {
	constexpr Vector2 DisplaySizeDefault = Vector2{640, 480};
	String displayNameDefault = String{"Ona"};
	PackedStack<Module *> modules = {Allocator::Default};
	String modulesPath = {"./modules/"};

	RegisterImageLoader(String{"bmp"}, LoadBitmap);

	EnumeratePath(modulesPath, [&](String const & fileName) {
		if (fileName.EndsWith(String{".so"})) {
			modules.Push(new NativeModule{
				Allocator::Default,
				String::Concat({modulesPath, fileName})
			});

			return;
		}
	});

	GraphicsServer * graphicsServer = nullptr;

	{
		Config config = {Allocator::Default};
		String graphics = {"Graphics"};
		SystemStream stream = {};

		if (stream.Open(String{"ona.cfg"}, Stream::OpenRead)) {
			String configSource = {};

			if (LoadText(&stream, Allocator::Default, &configSource)) {
				String errorMessage = {};

				if (!config.Parse(configSource, &errorMessage)) {
					Print(errorMessage);
				}
			}
		}

		Vector2 initialDisplaySize = config.ReadVector2(
			graphics,
			String{"displaySize"},
			DisplaySizeDefault
		);

		String graphicsServerName = config.ReadString(graphics, String{"server"}, String{"opengl"});

		if (graphicsServerName.Equals(String{"opengl"}))
		{
			graphicsServer = LoadOpenGL(
				config.ReadString(graphics, String{"displayTitle"}, displayNameDefault),
				initialDisplaySize.x,
				initialDisplaySize.y
			);
		}
	}

	if (graphicsServer) {
		OnaEvents events = {};

		InitScheduler();

		modules.ForEach([&](Module * module) {
			module->Initialize();
		});

		while (graphicsServer->ReadEvents(&events)) {
			graphicsServer->Clear();

			modules.ForEach([&events](Module * module) {
				module->Process(events);
			});

			WaitAllTasks();
			graphicsServer->Update();
		}

		modules.ForEach([](Module * module) {
			module->Finalize();

			delete module;
		});
	}
}
