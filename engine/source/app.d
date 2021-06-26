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

private void main() {
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
				Sprite[1024] sprites = void;

				foreach (ref value; sprites) {
					value = Sprite.centered(
						testTexture.dimensions().asVector2(),
						Vector2(uniform(0, 1280), uniform(0, 720))
					);
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
