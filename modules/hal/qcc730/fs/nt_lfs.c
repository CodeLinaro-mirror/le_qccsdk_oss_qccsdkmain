/**
 */

#include "nt_flags.h"

#ifdef NT_FN_LFS

#include <stdarg.h>  // va_arg

#include "nt_mem.h"
#include "lfs.h"
#include "lfs_util.h"
#include "nt_lfs.h"
#include "uart.h"
#include "nt_common.h"
#if (NT_CHIP_VERSION==2)
#include "nt_devcfg.h"
#endif //(NT_CHIP_VERSION==2)
#include "nt_sys_monitoring.h"
#include "sys_onbd_cfg.h"

extern unsigned int _ln_FS_Start_Addr ; //File system mount address provided by linker

#define NT_FS_ST_ADDRESS ( (uint32_t)(&_ln_FS_Start_Addr) )


#define NT_ONE_BLOCK_SIZE ( 512 ) // 512 bytes for one block
#define NT_TOTAL_BLOCK_COUNT ( 64 )	 	// 64 * 512 = 32KB
#define NT_FS_TOTAL_SIZE ( NT_TOTAL_BLOCK_COUNT * NT_ONE_BLOCK_SIZE )
#define NT_SIZE_IN_KB			( NT_FS_TOTAL_SIZE >> 10 )

#define NT_READ_SIZE		( 16 )
#define NT_WRITE_SIZE		( NT_READ_SIZE )
#define NT_CACHE_SIZE		( NT_WRITE_SIZE )
#define NT_LOOKHEAD_SIZE	( NT_CACHE_SIZE )
#define NT_BLOCK_CYCLES		( NT_LOOKHEAD_SIZE )

#define NT_ONE_KB			( 1024 )
#define PBL_LOG_SIZE        ( NT_ONE_KB )

struct lfs_config cfg;

extern char pbl_log_buff[256];

int nt_lfs_block_read( const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size )
{
	int r_status;
	uint32_t address = (uint32_t)(NT_FS_ST_ADDRESS + ( block*c->block_size ) + off);
	r_status = nt_rram_read( address, buffer, size );
	return r_status;
}

int nt_lfs_block_prog( const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size )
{
	int w_status;
	const uint32_t *src = (const uint32_t *)(buffer);
	uint32_t address = (uint32_t)(NT_FS_ST_ADDRESS + ( block*c->block_size ) + off);
	w_status = nt_rram_write( address, (uint32_t *)(src), size);
	return w_status;
}

int nt_lfs_block_erase( const struct lfs_config *c, lfs_block_t block )
{
	uint32_t *address = (uint32_t*)(NT_FS_ST_ADDRESS + ( block*c->block_size ));
	uint8_t block_erase[16];
	int8_t rst_err;

	memset( &block_erase[0], 0xFF, sizeof(block_erase) );

	for ( uint16_t rst_count = 0; rst_count < (c->block_size >> 4); rst_count++ )
	{
		rst_err = nt_rram_write( (uint32_t)(address), &block_erase[0], sizeof(block_erase));
		if ( rst_err != 0 )
		{
#ifdef NT_DEBUG
			nt_dbg_print("block erase failed\r\n");
#endif
		}

		address += 4;
	}

	return 0;
}

int nt_lfs_block_sync( const struct lfs_config *c )
{
	(void)c;
	return 0;
}

int nt_lfs_pbl_logs(void)
{
	lfs_file_t pbl_crt_file;
	int err;
	err = lfs_file_open( &lfs_init, &pbl_crt_file, "PBL_log.txt", LFS_O_RDWR | LFS_O_CREAT);
	if(err != 0)
	{
		nt_uart_app_printf("Error opening PBL_log.txt\r\n");
		lfs_file_close( &lfs_init, &pbl_crt_file );
		return err;
	}
	else
	{
		int err_seek,err_read,err_write;

		uint16_t log_overlap_index[2] = {0, 0};

		uint32_t filesize = nt_find_filesize("PBL_log.txt");
		if( filesize == 0 )
		{
			if( pbl_log_buff[0] != 0 )
			{
				//sequence index increment.
				log_overlap_index[1] += 1;

				// writing to rram location for sequence index.
				err_write = lfs_file_write( &lfs_init, &pbl_crt_file, &log_overlap_index[0], sizeof(log_overlap_index) );

				if( err_write == 0 )
				{
					lfs_file_close( &lfs_init, &pbl_crt_file );
					return err_write;
				}
				//Decrementing the value of sequence index by 1 to get the offset index to store the log.
				log_overlap_index[1]--;

				err_seek = lfs_file_seek(&lfs_init,&pbl_crt_file,((pbl_log_buff[0] * log_overlap_index[1]) + sizeof(log_overlap_index)),LFS_SEEK_SET);
				if( err_seek == 0 )
				{
					lfs_file_close( &lfs_init, &pbl_crt_file );
					return err_seek;
				}

				err_write = lfs_file_write( &lfs_init, &pbl_crt_file, pbl_log_buff,pbl_log_buff[0]);
				if( err_write == 0 )
				{
					lfs_file_close( &lfs_init, &pbl_crt_file );
					return err_write;
	}

	lfs_file_close( &lfs_init, &pbl_crt_file );
}
			else
			{
				lfs_file_close( &lfs_init, &pbl_crt_file );
				nt_uart_app_printf(" \r\n !!!No PBL Log!!! \r\n");
				return 0;
			}
		}
		else
		{
			err_read = lfs_file_read( &lfs_init, &pbl_crt_file, &log_overlap_index[0], sizeof(log_overlap_index) );
			err_seek = lfs_file_seek(&lfs_init,&pbl_crt_file,0,LFS_SEEK_SET);
			if( err_read == 0 )
			{
				lfs_file_close( &lfs_init, &pbl_crt_file );
				return err_read;
			}

			if( pbl_log_buff[0] != 0 )// checking the pbl_log frame size
			{

				if( (log_overlap_index[1]) >= (PBL_LOG_SIZE/pbl_log_buff[0]) ) // checking the sequence index.
				{
					// overlap index incremented.
					log_overlap_index[0] += 1;
					// sequence index reset.
					log_overlap_index[1] = 1;

					// writing to rram location for overlap index
					err_write = lfs_file_write( &lfs_init, &pbl_crt_file, &log_overlap_index[0], sizeof(log_overlap_index) );
					if( err_write == 0 )
					{
						lfs_file_close( &lfs_init, &pbl_crt_file );
						return err_write;
					}

					log_overlap_index[1] = 0;
				}
				else
				{
					//sequence index increment.
					log_overlap_index[1] += 1;

					// writing to rram location for sequence index.
					err_write = lfs_file_write( &lfs_init, &pbl_crt_file, log_overlap_index, sizeof(log_overlap_index) );

					if( err_write == 0 )
					{
						lfs_file_close( &lfs_init, &pbl_crt_file );
						return err_write;
					}
					//Decrementing the value of sequence index by 1 to get the offset index to store the log.
					log_overlap_index[1]--;
				}

				err_seek = lfs_file_seek(&lfs_init,&pbl_crt_file,((pbl_log_buff[0] * log_overlap_index[1]) + sizeof(log_overlap_index)),LFS_SEEK_SET);
				if( err_seek == 0 )
				{
					lfs_file_close( &lfs_init, &pbl_crt_file );
					return err_seek;
				}

				err_write = lfs_file_write( &lfs_init, &pbl_crt_file, pbl_log_buff,pbl_log_buff[0]);
				if( err_write == 0 )
				{
					lfs_file_close( &lfs_init, &pbl_crt_file );
					return err_write;
				}

				lfs_file_close( &lfs_init, &pbl_crt_file );
			}
			else
			{
				lfs_file_close( &lfs_init, &pbl_crt_file );
			}

		}
	}
	return 0;
}


int nt_lfs_init( void )
{
	int err_code;
#if (NT_CHIP_VERSION==2)
	// Neutrino_2 code
	uint8_t _nt2_rram_dxe ;
/* N2 rram dxe enable or disable through devcfg */
	  if(nt_status_enable_disable_rram_dxe())
	  {
		  _nt2_rram_dxe = *((uint8_t*)(nt_devcfg_get_config(NT2_DEVCFG_ENABLE_DISABLE_RRAM_DXE)));
	  }
	  else
	  {
		  _nt2_rram_dxe = 0;
	  }
	if(_nt2_rram_dxe == 1)
	{
		// todo call the dxe enable function
		nt_rram__dxe_config();
	}
	else
	{
		;// todo call the dxe disable function
	}
#endif //(NT_CHIP_VERSION==2)
	// Neutrino_2 code
	cfg.read = nt_lfs_block_read;
	cfg.prog = nt_lfs_block_prog;
	cfg.erase = nt_lfs_block_erase;
	cfg.sync = nt_lfs_block_sync;

	cfg.read_size = NT_READ_SIZE; // 16 byte will be read on every read call.
	cfg.prog_size = NT_WRITE_SIZE; // 16 byte will be write on every write call.
	cfg.block_size = NT_ONE_BLOCK_SIZE; // Block size is 512 byte i.e 0.5KB
	cfg.block_count = NT_TOTAL_BLOCK_COUNT; // total size of blocks 64*512 = 32768, 32768/512 = 64 count
	cfg.cache_size = NT_CACHE_SIZE;
	cfg.lookahead_size = NT_LOOKHEAD_SIZE;
	cfg.block_cycles = NT_BLOCK_CYCLES;
	cfg.read_buffer = NULL;
	cfg.prog_buffer = NULL;
	cfg.lookahead_buffer = NULL;

	err_code = lfs_mount( &lfs_init, &cfg );
	if( err_code )
	{
		nt_uart_app_printf("\rMount Error: %d\r\n", err_code);
		err_code = lfs_format( &lfs_init, &cfg );
		if( err_code )
		{
			nt_uart_app_printf("\rFormat Error: %d\r\n", err_code);
		}
		else
		{
			nt_uart_app_printf("\rFormat Success: %d\r\n", err_code);
		}
		err_code = lfs_mount( &lfs_init, &cfg );
		if( err_code )
		{
			nt_uart_app_printf("\rMount Error: %d\r\n", err_code);
		}
		else
		{
			nt_uart_app_printf("\rMount Success: %d\r\n", err_code);
		}
	}
	else
	{
		nt_uart_app_printf("\r-->Mount Success: %d\r\n", err_code);
	}

//	if(err_code == 0)
//	{
//		nt_lfs_pbl_logs();
//	}

	return err_code;
}

uint32_t nt_find_filesize( const char *file )
{
	lfs_dir_t list_dir;
	struct lfs_info list_info;
	uint16_t list_count = 0;

	int err_dir = lfs_dir_open(&lfs_init, &list_dir, "/");
	if (err_dir) {
		nt_uart_app_printf("diropen error: %d\r\n", err_dir);
		return 0;
	}
	else
	{
		err_dir = lfs_dir_read(&lfs_init, &list_dir, &list_info);
		if (err_dir == 0) {
			nt_uart_app_printf("No file available: %d\n", err_dir);
		} else if (err_dir < 0) {
			nt_uart_app_printf("Read Directory Fail: %d\n", err_dir);
		}else{
			err_dir = lfs_dir_read(&lfs_init, &list_dir, &list_info);
			while (err_dir) {
				err_dir = lfs_dir_read(&lfs_init, &list_dir, &list_info);
				if (err_dir != 1) {
					nt_uart_app_printf("No file available: %d\r\n", err_dir);
					lfs_dir_close( &lfs_init, &list_dir );
					break;
				}
				list_count++;

				if( strcmp( file, (const char *)list_info.name) == 0 )
				{
					nt_uart_app_printf(" %d\t%s\t %d Bytes\r\n", list_count,
										list_info.name, list_info.size);
					lfs_dir_close( &lfs_init, &list_dir );
					return list_info.size;
				}
			}

		}
	}

	return 0;
}

/*
 * This command will unmount the file system and reset the file system
 * address space to 0x00 and followed by system software reset to
 * reboot the system
 * */
int32_t nt_factory_reset( void )
{

	uint8_t read_status = read_and_write_obd_cfg(CONFIG_READ) ;

	int32_t err_unmount = lfs_unmount(&lfs_init);

	if (err_unmount) {
		nt_uart_app_printf("Unmount Error: %d\r\n", err_unmount);
	} else {
		uint32_t *address = (uint32_t *)(NT_FS_ST_ADDRESS);
		uint8_t block_reset[16];

		memset( &block_reset[0], 0x00, sizeof(block_reset) );

		for( uint16_t rst_count = 0; rst_count < (NT_FS_TOTAL_SIZE >> 4); rst_count++ )
		{
			err_unmount = nt_rram_write( (uint32_t)(address), &block_reset[0], sizeof(block_reset));
			if ( err_unmount != 0 )
			{
#ifdef NT_DEBUG
				nt_dbg_print("block reset failed\r\n");
#endif
			}

			address += 4;
		}
		nt_uart_app_printf("Unmount Success: %d\r\n", err_unmount);
	}

	nt_lfs_init();

	if(read_status == LFS_ERR_OK )
	read_and_write_obd_cfg(CONFIG_WRITE);

	nt_system_sw_reset();

	return err_unmount;
}
#endif
