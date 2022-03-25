#ifndef NNG_MQTT_QOS_DB_API_H
#define NNG_MQTT_QOS_DB_API_H

#include "core/nng_impl.h"
#include "mqtt_qos_db.h"

#ifdef NNG_SUPP_SQLITE

#define nni_qos_db_init(db) nni_mqtt_qos_db_init((sqlite3 **) &(db))
#define nni_qos_db_fini(db) nni_mqtt_qos_db_close((sqlite3 *) (db))
#define nni_qos_db_set(db, pipe_id, msg) \
	nni_mqtt_qos_db_set((sqlite3 *) (db), pipe_id, msg)
#define nni_qos_db_get(db, pipe_id) \
	nni_mqtt_qos_db_get((sqlite3 *) (db), pipe_id)
#define nni_qos_db_get_one(db, pipe_id) \
	nni_mqtt_qos_db_get_one((sqlite3 *) (db), &pipe_id)
#define nni_qos_db_remove(db, pipe_id) \
	nni_mqtt_qos_db_remove((sqlite3 *) (db), pipe_id)
#define nni_qos_db_remove_msg(db, msg)                             \
	{                                                          \
		nni_mqtt_qos_db_remove_msg((sqlite3 *) (db), msg); \
		nni_msg_free(msg);                                 \
	}
#define nni_qos_db_remove_all_msg(db, cb) \
	nni_mqtt_qos_db_remove_all_msg((sqlite3 *) (db))
#define nni_qos_db_foreach(db, cb) \
	nni_mqtt_qos_db_foreach((sqlite3 *) (db), cb)
#define nni_qos_db_check_remove_msg(db, msg) \
	nni_mqtt_qos_db_check_remove_msg((sqlite3 *) (db), msg)
#define nni_qos_db_reset_pipe(db) \
	nni_mqtt_qos_db_update_all_pipe((sqlite3 *) (db), 0)
#define nni_qos_db_set_pipe(db, pipe_id, client_id) \
	nni_mqtt_qos_db_set_pipe((sqlite3 *) db, pipe_id, client_id)

#else

#define nni_qos_db_init(db)                                         \
	{                                                           \
		(nni_id_map *) db = nng_zalloc(sizeof(nni_id_map)); \
		nni_id_map_init((nni_id_map *) db, 0, 0, false);    \
	}
#define nni_qos_db_fini(db)                                        \
	{                                                          \
		nni_id_map_fini((nni_id_map *) (db));              \
		nni_free((nni_id_map *) (db), sizeof(nni_id_map)); \
	}
#define nni_qos_db_set(db, pipe_id, msg) \
	nni_id_set((nni_id_map *) (db), pipe_id, msg)
#define nni_qos_db_get(db, pipe_id) nni_id_get((nni_id_map *) (db), pipe_id)
#define nni_qos_db_get_one(db, pipe_id) \
	nni_id_get_any((nni_id_map *) (db), &pipe_id)
#define nni_qos_db_remove(db, pipe_id) \
	nni_id_remove((nni_id_map *) (db), pipe_id)
#define nni_qos_db_remove_all_msg(db, cb) \
	nni_id_map_foreach((nni_id_map *) (db), cb)
#define nni_qos_db_foreach(db, cb) nni_id_map_foreach((nni_id_map *) (db), cb)
#define nni_qos_db_remove_msg(db, msg) nni_msg_free(msg)
#define nni_qos_db_check_remove_msg(db, msg) nni_msg_free(msg)
#define nni_qos_db_reset_pipe(db)
#define nni_qos_db_set_pipe(db, pipe_id, client_id)

#endif

#endif
