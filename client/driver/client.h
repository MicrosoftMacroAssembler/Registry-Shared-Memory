#pragma once

namespace client {
    struct data_initialize_t {
        std::uint32_t m_pid;
        std::uint64_t m_base_address;
    };

    struct data_request_t {
        std::uint32_t m_process_id;
        eprocess_t* m_eprocess;
        std::uint64_t m_address,
            m_address1;
        void* m_address2;
        std::size_t m_size;
    };

    enum e_request_type : std::uint32_t {
        ping_driver = 0,
        get_eprocess,
        get_base_address,
        get_directory_table_base,
        read_memory,
        write_memory
    };
}