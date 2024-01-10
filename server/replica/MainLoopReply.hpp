#pragma once

namespace dropbox::replica {
/// Reply from the main loop functions in either replicas.
enum class MainLoopReply: int8_t {
    kLostConnectionToPrimary = -1, ///< Backup lost connection to server.
    kShutdown, ///< A shutdown was issued to the current process.
};
}
