module app;

private import
	ona.config,
	ona.functional,
	ona.graphics,
	ona.image,
	ona.math,
	ona.string,
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
			auto test = Image.loadBmp(SystemStream.load("./actor/default.bmp").or([]));

			if (test.hasValue()) {
				auto noshirt = graphics.loadTexture(test.value());
				scope canvas = new Canvas();

				while (graphics.poll()) {
					graphics.clear(Color.black);
					canvas.drawSprite(noshirt, Vector2.zero);
					graphics.render(canvas);
				}

				noshirt.close();
			}
		}
	}
}
