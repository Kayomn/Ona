module ona.engine.app;

private import
	ona.core,
	ona.engine.graphics,
	ona.engine.sprites;

public void main() {
	GraphicsServer graphicsServer = loadGraphics(String("Ona").chars(), 640, 480);
	SpriteCommands spriteCommands = SpriteCommands.make(graphicsServer);
	Image image = Image.solid(null, Point2(32, 32), Color.white).valueOf();
	Sprite sprite = spriteCommands.createSprite(graphicsServer, image).valueOf();
	Events events;

	while (graphicsServer.readEvents(&events)) {
		graphicsServer.clear();
		spriteCommands.draw(sprite, Vector2(20, 20));
		spriteCommands.dispatch(graphicsServer);
		graphicsServer.update();
	}
}
