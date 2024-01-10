#pragma once

namespace dropbox {

/// Composite interface because C++ don't work well with circular references.
class CompositeInterface {
   public:
    virtual ~CompositeInterface()                                         = default;
    [[nodiscard]] virtual const std::string& GetUsername() const noexcept = 0;

    /**
     * Removes from the list a client by its ID.
     * @param id Client ID.
     * @warning Destroys the client pointed to by \p id.
     */
    virtual void Remove(int id) = 0;

    /**
     * Broadcasts an upload.
     * @param origin The device that sent to the server.
     * @param path The path of the file.
     * @return Whether the broadcast was a success.
     */
    virtual bool BroadcastUpload(int origin, const std::filesystem::path& path) = 0;

    /**
     * @brief Broadcasts a delete.
     * @copydoc BroadcastUpload
     */
    virtual bool BroadcastDelete(int origin, const std::filesystem::path& path) = 0;
};

}
