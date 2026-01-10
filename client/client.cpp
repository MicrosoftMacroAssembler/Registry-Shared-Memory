#include <iostream>
#include <Windows.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <powrprof.h>
#include <chrono>
#include <ctime>
#include <vector>
#include <tlhelp32.h>
#include <fstream>
#include <winternl.h>
#include <cstdint>
#include <DbgHelp.h>
#include <thread>
#include <functional>
#include <map>
#include <algorithm>
#pragma comment(lib, "powrprof.lib")

#include "driver/ia32.h"
#include "driver/client.h"
#include "driver/driver.hxx"
auto g_driver = std::make_unique< driver::c_driver >( );


int main( ) {
    if ( !g_driver->initialize( ) )
        return std::getchar( );

    std::getchar( );

    auto process_id = g_driver->get_process_id( L"notepad.exe" );
    if ( !process_id )
        return std::getchar( );

    std::cout << "process_id: " << process_id << std::endl;

    auto eprocess = g_driver->get_eprocess( process_id );
    if ( !eprocess )
        return std::getchar( );

    std::cout << "eprocess: " << eprocess << std::endl;

    auto base_address = g_driver->get_base_address( eprocess );
    if ( !base_address )
        return std::getchar( );

    std::cout << "base_address: " << base_address << std::endl;

    auto directory_table_base = g_driver->get_directory_table_base( base_address );
    if ( !directory_table_base )
        return std::getchar( );

    std::cout << "directory_table_base: " << directory_table_base << std::endl;

    std::uint64_t call_count = 0;
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency( &freq );
    QueryPerformanceCounter( &start );

    std::chrono::milliseconds duration = std::chrono::seconds( 1 );
    auto duration_ms = duration.count( );
    auto test_end = start.QuadPart + ( freq.QuadPart * duration_ms / 1000 );

    while ( true ) {
        QueryPerformanceCounter( &end );
        if ( end.QuadPart >= test_end )
            break;

        void* buffer;
        if ( !g_driver->read( base_address, &buffer, sizeof( std::uint64_t ) ) )
            std::cout << "failed to read" << std::endl;

        call_count++;
    }

    auto actual_duration = static_cast< double >( end.QuadPart - start.QuadPart ) / freq.QuadPart;
    auto reads_per_sec = static_cast< double >( call_count ) / actual_duration;

    printf( ( "Completed %llu calls in %.6f seconds\n" ), call_count, actual_duration );
    printf( ( "Rate: %.2f reads per second\n" ), reads_per_sec );

    return std::getchar( );
}