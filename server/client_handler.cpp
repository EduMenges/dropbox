#include "client_handler.hpp"

#include <dirent.h>
#include <unistd.h>

#include <filesystem>
#include <thread>
#include <vector>


#include "exceptions.hpp"
#include "list_directory.hpp"
#include "../common/utils.hpp"
#include "../common/inotify.hpp"
#include "../common/constants.hpp"

dropbox::ClientHandler::ClientHandler(int header_socket, int file_socket, int sync_sc_socket, int sync_cs_socket)
    : header_socket_(header_socket),
      file_socket_(file_socket),
      sync_sc_socket_(sync_sc_socket),
      sync_cs_socket_(sync_cs_socket),
      he_(header_socket),
      fe_(file_socket),
      de_(file_socket),
      sche_(sync_sc_socket),
      scfe_(sync_sc_socket),
      cshe_(sync_cs_socket),
      csfe_(sync_cs_socket),
      inotify_({}) {
    if (!ReceiveUsername()) {
        throw Username();
    }

    // Cria diretorio no servidor
    CreateUserFolder();

    // Puxa o diretorio para a maquina do client
    ReceiveGetSyncDir();

    // Monitora o diretorio
    /**
     * NAO FAÇO IDEIA DO PORQUÊ DISSO
     * mas tu tem que deixar um terminal do wsl aberto no deretorio que
     * ele vai escutar no windows, no linux nao sei como ta funcionando
    */
    inotify_ = Inotify(username_);
    std::thread inotify_thread_(
        [this](){
            inotify_.Start();
        }
    );

    // Thread que ennvia as informações para o client, client recebe essas infos
    // lá na funcao ReceiveSyncFromServer do client.cpp
    // Troca os arquivos
    std::thread file_exchange_thread(
        [this]() {
            while (true) {
                if (!inotify_.inotify_vector_.empty()) {
                    //std::string queue = inotify_.GetQueue();
                    std::string queue = inotify_.inotify_vector_.front();
                    inotify_.inotify_vector_.erase(inotify_.inotify_vector_.begin());
                    std::istringstream iss(queue);

                    std::string command;
                    std::string file;

                    iss >> command;
                    iss >> file;

                    std::cout << "Must att in Client | op:" << command << " in:" << file << '\n';

                    if (command == "write") {
                        if (!sche_.SetCommand(Command::WRITE_DIR).Send()) { }
                        
                        if (!scfe_.SetPath( SyncDirWithPrefix(username_) / file).SendPath()) { }

                        if (!scfe_.SetPath(std::move(SyncDirWithPrefix(username_) / file)).Send()) { }

                    } else if (command == "delete") {
                        if (!sche_.SetCommand(Command::DELETE_DIR).Send()) { }

                        if (!scfe_.SetPath(SyncDirWithPrefix(username_) / std::move(file)).SendPath()) {  }       
                    }
                }   
            }
        }
    );

    std::thread sync_thread(
        [this]() {
            while (true) {
                ReceiveSyncFromClient();
            }
        }
    );

    inotify_thread_.detach();
    file_exchange_thread.detach();
    sync_thread.detach();

    std::cout << "NEW CLIENT: " << username_ << '\n';
}

bool dropbox::ClientHandler::ReceiveUsername() {
    static thread_local std::array<char, NAME_MAX + 1> buffer{};

    if (!he_.Receive() || he_.GetCommand() != Command::USERNAME) {
        return false;
    }

    size_t username_length = 0;
    if (read(file_socket_, &username_length, sizeof(username_length)) != sizeof(username_length)) {
        perror(__func__);
        return false;
    }

    const auto kBytesReceived = read(file_socket_, buffer.data(), username_length);

    if (kBytesReceived != username_length) {
        perror(__func__);  // NOLINT
        return false;
    }

    username_ = std::string(buffer.data());
    return true;
}

void dropbox::ClientHandler::MainLoop() {
    bool receiving = true;

    while (receiving) {
        if (he_.Receive()) {
            std::cout << username_ << " ordered " << he_.GetCommand() << std::endl;  // NOLINT
            switch (he_.GetCommand()) {
                case Command::UPLOAD:
                    ReceiveUpload();
                    break;
                case Command::DOWNLOAD:
                    ReceiveDownload();
                    break;
                case Command::DELETE:
                    ReceiveDelete();
                    break;
                case Command::EXIT:
                    receiving = false;
                    break;
                case Command::LIST_SERVER:
                    ListServer();
                    break;
                default:
                    break;
            }
        }
    }
}

bool dropbox::ClientHandler::ReceiveUpload() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    return fe_.Receive();
}

bool dropbox::ClientHandler::ReceiveDelete() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    const std::filesystem::path& file_path = fe_.GetPath();

    if (std::filesystem::exists(file_path)) {
        std::filesystem::remove(file_path);
        return true;
    }

    return false;
}

bool dropbox::ClientHandler::ReceiveDownload() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    const std::filesystem::path& file_path = fe_.GetPath();

    if (!std::filesystem::exists(file_path)) {
        std::cerr << "Error: File does not exist - " << file_path << '\n';

        return he_.SetCommand(Command::ERROR).Send();
    }

    const bool kCouldSendSuccess = he_.SetCommand(Command::SUCCESS).Send();
    if (!kCouldSendSuccess) {
        return false;
    }

    return fe_.Send();
}

bool dropbox::ClientHandler::ReceiveGetSyncDir() {
    const std::filesystem::path        kSyncPath = SyncDirWithPrefix(username_);
    std::vector<std::filesystem::path> file_names;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(kSyncPath)) {
            if (std::filesystem::is_regular_file(entry.path())) {
                file_names.push_back(entry.path().filename());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return false;
    }

    for (const auto& file_name : file_names) {
        //std::cout << file_name << '\n';
        if (!he_.SetCommand(Command::SUCCESS).Send()) {
            return false;
        }

        if (!fe_.SetPath(kSyncPath / file_name.filename()).SendPath()) {
            return false;
        }

        if (!fe_.Send()) {
            return false;
        }
    }

    return he_.SetCommand(Command::EXIT).Send();
}

void dropbox::ClientHandler::CreateUserFolder() {
    try {
        if (!std::filesystem::exists(SyncDirWithPrefix(username_))) {
            std::filesystem::create_directory(SyncDirWithPrefix(username_));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory " << e.what() << '\n';
    }
}

dropbox::ClientHandler::~ClientHandler() {
    std::cout << username_ << " disconnected" << std::endl;  // NOLINT

    inotify_.Stop();

    //if (inotify_server_thread_.joinable()) {
    //    inotify_server_thread_.join();
    //}

    close(header_socket_);
    close(file_socket_);
    close(sync_sc_socket_);
    close(sync_cs_socket_);
}

bool dropbox::ClientHandler::ListServer() {
    std::string str_table = ListDirectory(SyncDirPath()).str();

    const size_t kTableSize = str_table.size() + 1;

    if (write(header_socket_, &kTableSize, sizeof(kTableSize)) == kInvalidWrite) {
        perror(__func__);
        return false;
    }

    size_t total_sent = 0;
    while (total_sent != kTableSize) {
        const size_t kBytesToSend = std::min(kPacketSize, kTableSize - total_sent);

        if (write(header_socket_, str_table.c_str() + total_sent, kBytesToSend) == kInvalidWrite) {
            perror(__func__);
            return false;
        }

        total_sent += kTableSize;
    }

    return true;
}

bool dropbox::ClientHandler::ReceiveSyncFromClient() {
    if (!cshe_.Receive()) {
        return false;
    }


    if (cshe_.GetCommand() == Command::WRITE_DIR) {
        //inotify_.Stop();
        inotify_.Pause();
        printf("CLIENT -> SERVER: modified\n");
        if (!csfe_.ReceivePath()) {
            return false;
        }
        std::cout << csfe_.GetPath() << '\n';

        if (!csfe_.Receive()) {
            return false;
        }

        //usleep(1000);
        //inotify_.inotify_vector_.erase(inotify_.inotify_vector_.begin());
        //inotify_ = Inotify("arthur");
        //std::thread inotify_thread_(
        //    [this]() {
        //        inotify_.Start();
        //    }
        //);
        //inotify_thread_.detach();
        inotify_.Resume();

        return true;
        
    } else if (cshe_.GetCommand() == Command::DELETE_DIR) {
        //inotify_.Stop();
        printf("CLIENT -> SERVER: delete\n");
        if (!csfe_.ReceivePath()) {
            return false;
        }

        const std::filesystem::path& file_path = csfe_.GetPath();

        if (std::filesystem::exists(file_path)) {
            std::filesystem::remove(file_path);
            
            //usleep(1000);
            //inotify_.inotify_vector_.erase(inotify_.inotify_vector_.begin());
            //inotify_ = Inotify("arthur");
            //std::thread inotify_thread_(
            //    [this]() {
            //        inotify_.Start();
            //    }
            //);
            //inotify_thread_.detach();

            return true;
        }

        return false;
    }

    return true;
}