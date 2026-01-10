#pragma once

namespace driver {
	class c_driver {
	public:
		bool initialize( ) {
            auto result = RegCreateKeyExW(
                HKEY_CURRENT_USER,
                L"SOFTWARE\\SharedMemory",
                0,
                nullptr,
                REG_OPTION_VOLATILE,
                KEY_WRITE | KEY_SET_VALUE,
                nullptr,
                &m_registry_key,
                nullptr
            );

            if ( result != ERROR_SUCCESS ) {
                std::cout << "Failed to create key. Error: " << result << std::endl;
                return false;
            }

            std::cout << "Key created successfully\n";

            m_data_request = reinterpret_cast< client::data_request_t* >(
                VirtualAlloc(
                    nullptr,
                    sizeof( client::data_request_t ),
                    MEM_COMMIT | MEM_RESERVE,
                    PAGE_READWRITE
                ) );

            if ( !m_data_request ) {
                std::cout << "Failed to allocate data. Error: " << GetLastError( ) << std::endl;
                return std::getchar( );
            }

            memset( m_data_request, 0, sizeof( client::data_request_t ) );

            SRWLOCK request_lock{ };
            InitializeSRWLock( &request_lock );

            client::data_initialize_t driver_initialize{};
            driver_initialize.m_pid = GetCurrentProcessId( );
            driver_initialize.m_base_address = reinterpret_cast< std::uint64_t >( m_data_request );

            result = RegSetValueExW(
                m_registry_key,
                L"InitializeSharedMemory",
                0,
                REG_BINARY,
                reinterpret_cast< const BYTE* >( &driver_initialize ),
                sizeof( driver_initialize )
            );

            if ( result == ERROR_SUCCESS ) {
                std::cout << "Communication setup successfully\n";
            }
            else {
                std::cout << "Failed to write value. Error: " << result << std::endl;
            }

            return true;
		}

        std::uint32_t get_process_id( std::wstring process_name ) {
            auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
            if ( snapshot == INVALID_HANDLE_VALUE )
                return false;

            PROCESSENTRY32W process_entry{ };
            process_entry.dwSize = sizeof( process_entry );
            Process32FirstW( snapshot, &process_entry );
            do {
                if ( !process_name.compare( process_entry.szExeFile ) )
                    return process_entry.th32ProcessID;
            } while ( Process32NextW( snapshot, &process_entry ) );

            return 0;
        }

        eprocess_t* get_eprocess( std::uint32_t process_id ) {
            m_data_request->m_process_id = process_id;

            auto request_type = client::e_request_type::get_eprocess;
            auto result = RegSetValueExW(
                m_registry_key,
                L"RequestSharedMemory",
                0,
                REG_BINARY,
                reinterpret_cast< const BYTE* >( &request_type ),
                sizeof( request_type )
            );

            if ( result == ERROR_SUCCESS ) {
                return m_data_request->m_eprocess;
            }
            else {
                std::cout << "Failed to write value. Error: " << result << std::endl;
            }

            return nullptr;
        }

        void* get_base_address( eprocess_t* eprocess ) {
            m_data_request->m_eprocess = eprocess;

            auto request_type = client::e_request_type::get_base_address;
            auto result = RegSetValueExW(
                m_registry_key,
                L"RequestSharedMemory",
                0,
                REG_BINARY,
                reinterpret_cast< const BYTE* >( &request_type ),
                sizeof( request_type )
            );

            if ( result == ERROR_SUCCESS ) {
                return m_data_request->m_address2;
            }
            else {
                std::cout << "Failed to write value. Error: " << result << std::endl;
            }

            return 0;
        }

        std::uint64_t get_directory_table_base( void* base_address ) {
            m_data_request->m_address2 = base_address;

            auto request_type = client::e_request_type::get_directory_table_base;
            auto result = RegSetValueExW(
                m_registry_key,
                L"RequestSharedMemory",
                0,
                REG_BINARY,
                reinterpret_cast< const BYTE* >( &request_type ),
                sizeof( request_type )
            );

            if ( result == ERROR_SUCCESS ) {
                return m_data_request->m_address;
            }
            else {
                std::cout << "Failed to write value. Error: " << result << std::endl;
            }

            return 0;
        }

        bool read_memory( std::uint64_t address, void* buffer, std::size_t size ) {
            m_data_request->m_address = address;
            m_data_request->m_address2 = buffer;
            m_data_request->m_size = size;

            auto request_type = client::e_request_type::read_memory;
            auto result = RegSetValueExW(
                m_registry_key,
                L"RequestSharedMemory",
                0,
                REG_BINARY,
                reinterpret_cast< const BYTE* >( &request_type ),
                sizeof( request_type )
            );

            return result == ERROR_SUCCESS;
        }

        bool write_memory( std::uint64_t address, void* buffer, std::size_t size ) {
            m_data_request->m_address = address;
            m_data_request->m_address2 = buffer;
            m_data_request->m_size = size;

            auto request_type = client::e_request_type::write_memory;
            auto result = RegSetValueExW(
                m_registry_key,
                L"RequestSharedMemory",
                0,
                REG_BINARY,
                reinterpret_cast< const BYTE* >( &request_type ),
                sizeof( request_type )
            );

            return result == ERROR_SUCCESS;
        }

        template <typename addr_t>
        bool read( addr_t va, void* buffer, size_t size ) {
            std::uint64_t va64;
            if constexpr ( std::is_pointer_v<addr_t> ) {
                va64 = reinterpret_cast< std::uint64_t >( va );
            }
            else if constexpr ( std::is_integral_v<addr_t> ) {
                va64 = static_cast< std::uint64_t >( va );
            }
            else {
                static_assert( std::is_pointer_v<addr_t> || std::is_integral_v<addr_t>,
                    "addr_t must be pointer or integral" );
            }

            if ( !read_memory( va64, buffer, size ) )
                return false;

            return true;
        }

        template <typename value_t, typename addr_t>
        bool write( addr_t va, const value_t& value ) {
            std::uint64_t va64;
            if constexpr ( std::is_pointer_v<addr_t> ) {
                va64 = reinterpret_cast< std::uint64_t >( va );
            }
            else if constexpr ( std::is_integral_v<addr_t> ) {
                va64 = static_cast< std::uint64_t >( va );
            }
            else {
                static_assert( std::is_pointer_v<addr_t> || std::is_integral_v<addr_t>,
                    "addr_t must be pointer or integral" );
            }

            if ( !write_memory( va64, const_cast< value_t* >( &value ), sizeof( value_t ) ) )
                return false;

            return true;
        }


        template <typename ret_t = std::uint64_t, typename addr_t>
        ret_t read_virt( addr_t va ) {
            std::uint64_t va64;
            if constexpr ( std::is_pointer_v<addr_t> ) {
                va64 = reinterpret_cast< std::uint64_t >( va );
            }
            else if constexpr ( std::is_integral_v<addr_t> ) {
                va64 = static_cast< std::uint64_t >( va );
            }
            else {
                static_assert( std::is_pointer_v<addr_t> || std::is_integral_v<addr_t>,
                    "addr_t must be pointer or integral" );
            }

            ret_t buffer{ };
            if ( !read_memory( va64, &buffer, sizeof( ret_t ) ) )
                return {};
            return buffer;
        }

        void unload( ) {
            RegCloseKey( m_registry_key );
            RegDeleteKeyW( HKEY_CURRENT_USER, L"SOFTWARE\\SharedMemory" );
        }

	private:
        HKEY m_registry_key;
        client::data_request_t* m_data_request;
	};
}