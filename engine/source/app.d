module app;

private import
	ona.config,
	ona.functional,
	ona.graphics,
	ona.image,
	ona.math,
	ona.string,
	std.random,
	ona.system;

public void main() {
	scope config = new Config();

	if (config.parse(cast(char[])SystemStream.load("./ona.cfg").or([]))) {
		auto loadedGraphics = Graphics.load(
			config.find("Graphics", "displayName").stringValue().or("Ona"),
			config.find("Graphics", "displayWidth").integerValue().or(640),
			config.find("Graphics", "displayHeight").integerValue().or(480),
		);

		if (loadedGraphics.hasValue()) {
			auto graphics = loadedGraphics.value();
			auto test = Image.loadBmp(SystemStream.load("./actor/actor.bmp").or([]));

			if (test.hasValue()) {
				auto noshirt = graphics.loadTexture(test.value());

				auto sprites = new Sprite[1024];

				foreach (ref value; sprites) {
					value.destinationRect.origin = Vector2(uniform(0, 1280), uniform(0, 720));
					value.destinationRect.extent = Vector2(noshirt.dimensions);
				}

				while (graphics.poll()) {
					graphics.clear(Color.black);
					graphics.drawSprites(noshirt, sprites);
					graphics.render();
				}

				noshirt.close();
			}
		}
	}
}
