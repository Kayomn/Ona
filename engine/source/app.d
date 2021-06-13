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
			auto testImage = Image.loadBmp(SystemStream.load("./actor/actor.bmp").or([]));

			if (testImage.hasValue()) {
				auto testTexture = graphics.loadTexture(testImage.value());
				auto sprites = new Sprite[1024];

				foreach (ref value; sprites) {
					value.destinationRect.origin = Vector2(uniform(0, 1280) - 32, uniform(0, 720) - 32);
					value.destinationRect.extent = testTexture.dimensions.asVector2();
				}

				while (graphics.poll()) {
					graphics.renderClear(Color.black);
					graphics.renderSprites(testTexture, sprites);
					graphics.present();
				}
			}
		}
	}
}
