#pragma once
#include <includes/includes.h>

bool entry_point( ) {
	nt::m_base_address = nt::get_base_address( );
	if ( !nt::m_base_address )
		return false;

	auto system_process = reinterpret_cast< eprocess_t* >( 
		nt::ps_initial_system_process( ) );
	if ( !system_process )
		return false;

	paging::m_system_dtb = eprocess::get_directory_table_base( system_process );
	paging::switch_target( paging::m_system_dtb );
	if ( !paging::dpm::initialize( ) )
		return false;

	unicode_string_t altitude;
	nt::rtl_init_unicode_string( &altitude, oxorany( L"450000" ) );

	ularge_integer_t cookie;
	auto result = nt::cm_register_callback_ex(
		handler::registry_callback,
		altitude,
		&cookie
		);

	nt::dbg_print( oxorany( "CmRegisterCallbackEx: 0x%x\n" ), result );
	return true;
}