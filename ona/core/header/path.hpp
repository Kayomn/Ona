
namespace Ona::Core {
	/**
	 * Extracts the last dot delimited portion of `path` as the extension, returning an empty
	 * `String` if one could not be parsed.
	 *
	 * "file.txt" -> "txt"
	 * "www.internet.com" -> "com"
	 * "user@email.io" -> "io"
	 */
	String PathExtension(String const & path);
}
