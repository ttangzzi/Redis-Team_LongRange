//server.h

/* Slave replication state. Used in server.repl_state for slaves to remember
 * what to do next. */
typedef enum {
    REPL_STATE_NONE = 0,            /* No active replication */
    REPL_STATE_CONNECT,             /* Must connect to master */
    REPL_STATE_CONNECTING,          /* Connecting to master */
    /* --- Handshake states, must be ordered --- */
    REPL_STATE_RECEIVE_PING_REPLY,  /* Wait for PING reply */
    REPL_STATE_RECEIVE_PANG_REPLY,  /* Wait for PANG reply                    PANG 코드 추가*/
   
    REPL_STATE_SEND_HANDSHAKE,      /* Send handshake sequence to master */
    REPL_STATE_RECEIVE_AUTH_REPLY,  /* Wait for AUTH reply */
    REPL_STATE_RECEIVE_PORT_REPLY,  /* Wait for REPLCONF reply */
    REPL_STATE_RECEIVE_IP_REPLY,    /* Wait for REPLCONF reply */
    REPL_STATE_RECEIVE_CAPA_REPLY,  /* Wait for REPLCONF reply */
    REPL_STATE_SEND_PSYNC,          /* Send PSYNC */
    REPL_STATE_RECEIVE_PSYNC_REPLY, /* Wait for PSYNC reply */
    /* --- End of handshake states --- */
    REPL_STATE_TRANSFER,        /* Receiving .rdb from master */
    REPL_STATE_CONNECTED,       /* Connected to master */
} repl_state;

/*Redis 슬레이브가 master서버와의 복제 프로세스를 수행하는 동안 어떠한 상태에 있는지를 추적
 REPL_STATE_RECEIVE_PANG_REPLY  이 부분이 PANG 응답을 기다리는 상태 */

/* 구조체에 pang, pung 추가 */
struct sharedObjectsStruct {
    robj *ok, *err, *emptybulk, *czero, *cone, *pong, *space, *pung,
    *queued, *null[4], *nullarray[4], *emptymap[4], *emptyset[4],
    *emptyarray, *wrongtypeerr, *nokeyerr, *syntaxerr, *sameobjecterr,
    *outofrangeerr, *noscripterr, *loadingerr,
    *slowevalerr, *slowscripterr, *slowmoduleerr, *bgsaveerr,
    *masterdownerr, *roslaveerr, *execaborterr, *noautherr, *noreplicaserr,
    *busykeyerr, *oomerr, *plus, *messagebulk, *pmessagebulk, *subscribebulk,
    *unsubscribebulk, *psubscribebulk, *punsubscribebulk, *del, *unlink,
    *rpop, *lpop, *lpush, *rpoplpush, *lmove, *blmove, *zpopmin, *zpopmax,
    *emptyscan, *multi, *exec, *left, *right, *hset, *srem, *xgroup, *xclaim,  
    *script, *replconf, *eval, *persist, *set, *pexpireat, *pexpire,
    *time, *pxat, *absttl, *retrycount, *force, *justid, *entriesread,
    *lastid, *ping, *pang, *setid, *keepttl, *load, *createconsumer,
    *getack, *special_asterick, *special_equals, *default_username, *redacted,
    *ssubscribebulk,*sunsubscribebulk, *smessagebulk,
    *select[PROTO_SHARED_SELECT_CMDS],
    *integers[OBJ_SHARED_INTEGERS],
    *mbulkhdr[OBJ_SHARED_BULKHDR_LEN], /* "*<value>\r\n" */
    *bulkhdr[OBJ_SHARED_BULKHDR_LEN],  /* "$<value>\r\n" */
    *maphdr[OBJ_SHARED_BULKHDR_LEN],   /* "%<value>\r\n" */
    *sethdr[OBJ_SHARED_BULKHDR_LEN];   /* "~<value>\r\n" */
    sds minstring, maxstring;
};
/*공유 객체를 관리하는 구조체 정의. */


/* redis 서버에 pang추가 */
int repl_pang_slave_period;     /* Master pangs the slave every N seconds */
/*마스터(master) 서버가 슬레이브(slave)에게 PANG 메시지를 보내는 주기를 나타냄,
 이 변수를 통해 마스터는 주기적으로 슬레이브에게 PANG 메시지를 보내어 통신이 활성화되어 있는지 확인할 수 있음*/


mstime_t cluster_pang_interval;/* A debug configuration for setting how often cluster nodes send pang messages. */
/*이 변수는 Redis 클러스터에서 노드 간에 PANG 메시지를 얼마나 자주 보낼지를 설정하는 디버깅(debugging) 구성*/

/* Commands prototypes */
void pangCommand(client *c);
/*"PANG" 을 처리하는 함수*/