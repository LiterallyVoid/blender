/**
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Contributor(s): Peter Schlaile <peter@schlaile.de> 2005
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef __MEM_cache_limiter_c_api_h_included__
#define __MEM_cache_limiter_c_api_h_included__ 1

#ifdef __cplusplus
extern "C" {
#endif
	
struct MEM_CacheLimiter_s;
struct MEM_CacheLimiterHandle_s;

typedef struct MEM_CacheLimiter_s MEM_CacheLimiterC;
typedef struct MEM_CacheLimiterHandle_s MEM_CacheLimiterHandleC;

/* function used to remove data from memory */
typedef void(*MEM_CacheLimiter_Destruct_Func)(void*);

#ifndef __MEM_cache_limiter_h_included__
extern void MEM_CacheLimiter_set_maximum(int m);
extern int MEM_CacheLimiter_get_maximum();
#endif
/** 
 * Create new MEM_CacheLimiter object 
 * managed objects are destructed with the data_destructor
 *
 * @param data_destructor
 * @return A new MEM_CacheLimter object
 */

extern MEM_CacheLimiterC * new_MEM_CacheLimiter(
	MEM_CacheLimiter_Destruct_Func data_destructor);

/** 
 * Delete MEM_CacheLimiter
 * 
 * Frees the memory of the CacheLimiter but does not touch managed objects!
 *
 * @param This "This" pointer
 */

extern void delete_MEM_CacheLimiter(MEM_CacheLimiterC * This);

/** 
 * Manage object
 * 
 * @param This "This" pointer, data data object to manage
 * @return CacheLimiterHandle to ref, unref, touch the managed object
 */
	
extern MEM_CacheLimiterHandleC * MEM_CacheLimiter_insert(
	MEM_CacheLimiterC * This, void * data);

/** 
 * Free objects until memory constraints are satisfied
 * 
 * @param This "This" pointer
 */

extern void MEM_CacheLimiter_enforce_limits(MEM_CacheLimiterC * This);

/** 
 * Unmanage object previously inserted object. 
 * Does _not_ delete managed object!
 * 
 * @param This "This" pointer, handle of object
 */
	
extern void MEM_CacheLimiter_unmanage(MEM_CacheLimiterHandleC * handle);


/** 
 * Raise priority of object (put it at the tail of the deletion chain)
 * 
 * @param handle of object
 */
	
extern void MEM_CacheLimiter_touch(MEM_CacheLimiterHandleC * handle);

/** 
 * Increment reference counter. Objects with reference counter != 0 are _not_
 * deleted.
 * 
 * @param handle of object
 */
	
extern void MEM_CacheLimiter_ref(MEM_CacheLimiterHandleC * handle);

/** 
 * Decrement reference counter. Objects with reference counter != 0 are _not_
 * deleted.
 * 
 * @param handle of object
 */
	
extern void MEM_CacheLimiter_unref(MEM_CacheLimiterHandleC * handle);

/** 
 * Get reference counter.
 * 
 * @param This "This" pointer, handle of object
 */
	
extern int MEM_CacheLimiter_get_refcount(MEM_CacheLimiterHandleC * handle);

/** 
 * Get pointer to managed object
 * 
 * @param handle of object
 */
	
extern void * MEM_CacheLimiter_get(MEM_CacheLimiterHandleC * handle);

#ifdef __cplusplus
}
#endif


#endif

