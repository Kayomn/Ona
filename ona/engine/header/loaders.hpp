
namespace Ona::Engine {
	using namespace Ona::Core;
	using namespace Ona::Engine;

	using ImageLoader = Callable<
		Error<ImageError>(Allocator * imageAllocator, File & file, Image & result)
	>;

	/**
	 * Searches for an image loader for loading files matching the file extension `fileExtension`,
	 * returning `nullptr` if none match the specified file extension.
	 */
	ImageLoader const * FindImageLoader(String const & fileExtension);

	/**
	 * Attempts to register `imageLoader` as the image loading function for images loaded from any
	 * file path that ends with `fileExtension`.
	 *
	 * Registering an `imageLoader` with a pre-existing `fileExtension` will replace that image
	 * loading strategy.
	 *
	 * Successfully registering the image will return `true`, otherwise `false` is returned if it
	 * could not be registered due to a lack of available system resources.
	 */
	bool RegisterImageLoader(String const & fileExtension, ImageLoader const & imageLoader);

	/**
	 * Loads the text contents of `file` into memory using `tempAllocator` to allocate the temporary
	 * buffer used in between reading it into memory and converting the memory contents to a
	 * `String`.
	 */
	String LoadText(Allocator * tempAllocator, File & file);

	/**
	 * Attempts to load the contents of `file` into memory as a bitmap using `imageAllocator` as the
	 * dynamic memory allocation strategy for the `Image`.
	 */
	Error<ImageError> LoadBitmap(Allocator * imageAllocator, File & file, Image & result);
}
