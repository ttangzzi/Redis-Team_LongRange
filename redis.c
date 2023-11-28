// server.c 코드 추가

/* Shared command argument */
shared.pang = createStringObject("pang",4);  
/* 서버 초기화 단계에서 명령어를 초기화, pang이라는 Redis문자열 객체를 생성 후 share.pang에 할당
"PANG" 명령어에 응답하는 데 사용. */


/* pang */
if (deny_write_type != DISK_ERROR_TYPE_NONE &&
    (is_write_command || c->cmd->proc == pangCommand))
{
    if (obey_client) {
        if (!server.repl_ignore_disk_write_error && c->cmd->proc != pangCommand) {
            serverPanic("Replica was unable to write command to disk.");
        } else {
            static mstime_t last_log_time_ms = 0;
            const mstime_t log_interval_ms = 10000;
            if (server.mstime > last_log_time_ms + log_interval_ms) {
                last_log_time_ms = server.mstime;
                serverLog(LL_WARNING, "Replica is applying a command even though "
                                      "it is unable to write to disk.");
            }
        }
    } else {
        sds err = writeCommandsGetDiskErrorMessage(deny_write_type);
        /* remove the newline since rejectCommandSds adds it. */
        sdssubstr(err, 0, sdslen(err)-2);
        rejectCommandSds(c, err);
        return C_OK;
    }
}    
/* 현재 명령어가 쓰기 명령어이거나 현재 명령어가 "pang" 명령어인 경우에는 다음과 같은 처리를 수행

obey_client가 true이면 (즉, Replication 중이라면) 디스크 쓰기 오류를 무시하지 않고, 현재 명령어가 "pang" 명령어가 아니라면 서버를 패닉 상태로 변환
그렇지 않으면, 주기적으로 로그를 출력. 즉, 디스크에 쓰지 못하는데도 불구하고 명령어를 계속 처리하고 있음을 경고로 기록
obey_client가 false이면 (즉, Replication이 아니라면)
디스크 쓰기 오류에 대한 에러 메시지를 가져오고, 줄 바꿈 문자를 제외한 에러 메시지를 이용하여 명령을 거부 */


if ((c->flags & CLIENT_PUBSUB && c->resp == 2) &&
    c->cmd->proc != pingCommand &&
    c->cmd->proc != pangCommand &&
    c->cmd->proc != subscribeCommand &&
    c->cmd->proc != ssubscribeCommand &&
    c->cmd->proc != unsubscribeCommand &&
    c->cmd->proc != sunsubscribeCommand &&
    c->cmd->proc != psubscribeCommand &&
    c->cmd->proc != punsubscribeCommand &&
    c->cmd->proc != quitCommand &&
    c->cmd->proc != resetCommand) {
    rejectCommandFormat(c,
        "Can't execute '%s': only (P|S)SUBSCRIBE / "
        "(P|S)UNSUBSCRIBE / PING / PANG / QUIT / RESET are allowed in this context",
        c->cmd->fullname);
    return C_OK;
}
/* c->flags & CLIENT_PUBSUB는 클라이언트가 Pub/Sub 모드에 있는지 여부를 확인, 또한 c->resp == 2는 클라이언트가 RESP2 프로토콜을 사용하는지 확인 
 위의 내용에 정의된 명령어가 아니면 rejectCommandFormat 함수를 통해 오류 message를 출력 */

/* pang 명령어 처리 함수 */
void pangCommand(client *c) {
  /* The command takes zero or one arguments. */
  if (c->argc > 2) {
      addReplyErrorArity(c);
      return;
  }

  if (c->flags & CLIENT_PUBSUB && c->resp == 2) {
      addReply(c,shared.mbulkhdr[2]);
      addReplyBulkCBuffer(c,"pung",4);
      if (c->argc == 1)
          addReplyBulkCBuffer(c,"",0);
      else
          addReplyBulk(c,c->argv[1]);
  } else {
      if (c->argc == 1)
          addReply(c,shared.pung);
      else
          addReplyBulk(c,c->argv[1]);
  }
}    
/* 인자의 개수 0 또는 1개로 제한, 멀티 벌크 응답 헤더와 pung 문자열로 시작하며 인자가 제공되면 인자를 멀티 벌크 응답으로 추가.
 그 후 클라이언트에 응답 전송*/


/* Shared command responses */
shared.pung = createObject(OBJ_STRING,sdsnew("+PUNG\r\n"));
/*문자열 객체(OBJ_STRING)를 생성하고, 그 값을 "PUNG" 문자열에 대한 RESP(서버와 클라 간의 간단하면서 효율적인 이진 프로토콜) 형식으로 설정 */




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


//redis-cli.c 

typedef struct clusterManagerNode {
    redisContext *context;
    sds name;
    char *ip;
    int port;
    int bus_port; /* cluster-port */
    uint64_t current_epoch;
    time_t ping_sent;
    time_t ping_recv;
    time_t pang_sent; /* pang_sent와 pang_recv추가 */
    time_t pang_recv;
    int flags;
    list *flags_str; /* Flags string representations */
    sds replicate;  /* Master ID if node is a slave */
    int dirty;      /* Node has changes that can be flushed */
    uint8_t slots[CLUSTER_MANAGER_SLOTS];
    int slots_count;
    int replicas_count;
    list *friends;
    sds *migrating; /* An array of sds where even strings are slots and odd
                     * strings are the destination node IDs. */
    sds *importing; /* An array of sds where even strings are slots and odd
                     * strings are the source node IDs. */
    int migrating_count; /* Length of the migrating array (migrating slots*2) */
    int importing_count; /* Length of the importing array (importing slots*2) */
    float weight;   /* Weight used by rebalance */
    int balance;    /* Used by rebalance */
} clusterManagerNode;
/*클러스터 구조체 정의. 이 구조체에 PANG 메시지를 보낸 시간과 응답을 받은 시간을 나타냄*/



static clusterManagerNode *clusterManagerNewNode(char *ip, int port, int bus_port) {
    clusterManagerNode *node = zmalloc(sizeof(*node));
    node->context = NULL;
    node->name = NULL;
    node->ip = ip;
    node->port = port;
    /* We don't need to know the bus_port, at this point this value may be wrong.
     * If it is used, it will be corrected in clusterManagerLoadInfoFromNode. */
    node->bus_port = bus_port ? bus_port : port + CLUSTER_MANAGER_PORT_INCR;
    node->current_epoch = 0;
    node->ping_sent = 0;
    node->ping_recv = 0;
    node->pang_sent = 0; /* pang 추가 */
    node->pang_recv = 0;    
    node->flags = 0;
    node->flags_str = NULL;
    node->replicate = NULL;
    node->dirty = 0;
    node->friends = NULL;
    node->migrating = NULL;
    node->importing = NULL;
    node->migrating_count = 0;
    node->importing_count = 0;
    node->replicas_count = 0;
    node->weight = 1.0f;
    node->balance = 0;
    clusterManagerNodeResetSlots(node);
    return node;
}
/*새로운 노드를 생성하는 함수에 PANG을 추가하여 PANG메시지를 보낸 시간과 응답받은 시간을 초기화*/


static int clusterManagerNodeLoadInfo(clusterManagerNode *node, int opts,
                                      char **err) /* switch 문과 if 문 pang 추가 */
{
    redisReply *reply = CLUSTER_MANAGER_COMMAND(node, "CLUSTER NODES");
    int success = 1;
    *err = NULL;
    if (!clusterManagerCheckRedisReply(node, reply, err)) {
        success = 0;
        goto cleanup;
    }
    int getfriends = (opts & CLUSTER_MANAGER_OPT_GETFRIENDS);
    char *lines = reply->str, *p, *line;
    while ((p = strstr(lines, "\n")) != NULL) {
        *p = '\0';
        line = lines;
        lines = p + 1;
        char *name = NULL, *addr = NULL, *flags = NULL, *master_id = NULL,
             *ping_sent = NULL, *ping_recv = NULL, *pang_sent = NULL, *pang_recv = NULL, *config_epoch = NULL,  //pang 보낸 시간과 받은 시간 저장 변수 초기화   
             *link_status = NULL;
        UNUSED(link_status);
        int i = 0;
        while ((p = strchr(line, ' ')) != NULL) {
            *p = '\0';
            char *token = line;
            line = p + 1;
            switch(i++){
            case 0: name = token; break;
            case 1: addr = token; break;
            case 2: flags = token; break;
            case 3: master_id = token; break;
            case 4: ping_sent = token; break;
            case 5: ping_recv = token; break;
            case 6: config_epoch = token; break;
            case 7: link_status = token; break;
            case 8: pang_sent = token; break; /*pang 추가 후 if문 break 조건 8-> 10 수정*/
            case 9: pang_recv = token; break;
            }
            if (i == 10) break; // Slots
        }
        if (!flags) {
            success = 0;
            goto cleanup;
        }

        char *ip = NULL;
        int port = 0, bus_port = 0;
        if (addr == NULL || !parseClusterNodeAddress(addr, &ip, &port, &bus_port)) {
            fprintf(stderr, "Error: invalid CLUSTER NODES reply\n");
            success = 0;
            goto cleanup;
        }

        int myself = (strstr(flags, "myself") != NULL);
        clusterManagerNode *currentNode = NULL;
        if (myself) {
            /* bus-port could be wrong, correct it here, see clusterManagerNewNode. */
            node->bus_port = bus_port;
            node->flags |= CLUSTER_MANAGER_FLAG_MYSELF;
            currentNode = node;
            clusterManagerNodeResetSlots(node);
            if (i == 8) {
                int remaining = strlen(line);
                while (remaining > 0) {
                    p = strchr(line, ' ');
                    if (p == NULL) p = line + remaining;
                    remaining -= (p - line);

                    char *slotsdef = line;
                    *p = '\0';
                    if (remaining) {
                        line = p + 1;
                        remaining--;
                    } else line = p;
                    char *dash = NULL;
                    if (slotsdef[0] == '[') {
                        slotsdef++;
                        if ((p = strstr(slotsdef, "->-"))) { // Migrating
                            *p = '\0';
                            p += 3;
                            char *closing_bracket = strchr(p, ']');
                            if (closing_bracket) *closing_bracket = '\0';
                            sds slot = sdsnew(slotsdef);
                            sds dst = sdsnew(p);
                            node->migrating_count += 2;
                            node->migrating = zrealloc(node->migrating,
                                (node->migrating_count * sizeof(sds)));
                            node->migrating[node->migrating_count - 2] =
                                slot;
                            node->migrating[node->migrating_count - 1] =
                                dst;
                        }  else if ((p = strstr(slotsdef, "-<-"))) {//Importing
                            *p = '\0';
                            p += 3;
                            char *closing_bracket = strchr(p, ']');
                            if (closing_bracket) *closing_bracket = '\0';
                            sds slot = sdsnew(slotsdef);
                            sds src = sdsnew(p);
                            node->importing_count += 2;
                            node->importing = zrealloc(node->importing,
                                (node->importing_count * sizeof(sds)));
                            node->importing[node->importing_count - 2] =
                                slot;
                            node->importing[node->importing_count - 1] =
                                src;
                        }
                    } else if ((dash = strchr(slotsdef, '-')) != NULL) {
                        p = dash;
                        int start, stop;
                        *p = '\0';
                        start = atoi(slotsdef);
                        stop = atoi(p + 1);
                        node->slots_count += (stop - (start - 1));
                        while (start <= stop) node->slots[start++] = 1;
                    } else if (p > slotsdef) {
                        node->slots[atoi(slotsdef)] = 1;
                        node->slots_count++;
                    }
                }
            }
            node->dirty = 0;
        } else if (!getfriends) {
            if (!(node->flags & CLUSTER_MANAGER_FLAG_MYSELF)) continue;
            else break;
        } else {
            currentNode = clusterManagerNewNode(sdsnew(ip), port, bus_port);
            currentNode->flags |= CLUSTER_MANAGER_FLAG_FRIEND;
            if (node->friends == NULL) node->friends = listCreate();
            listAddNodeTail(node->friends, currentNode);
        }
        if (name != NULL) {
            if (currentNode->name) sdsfree(currentNode->name);
            currentNode->name = sdsnew(name);
        }
        if (currentNode->flags_str != NULL)
            freeClusterManagerNodeFlags(currentNode->flags_str);
        currentNode->flags_str = listCreate();
        int flag_len;
        while ((flag_len = strlen(flags)) > 0) {
            sds flag = NULL;
            char *fp = strchr(flags, ',');
            if (fp) {
                *fp = '\0';
                flag = sdsnew(flags);
                flags = fp + 1;
            } else {
                flag = sdsnew(flags);
                flags += flag_len;
            }
            if (strcmp(flag, "noaddr") == 0)
                currentNode->flags |= CLUSTER_MANAGER_FLAG_NOADDR;
            else if (strcmp(flag, "disconnected") == 0)
                currentNode->flags |= CLUSTER_MANAGER_FLAG_DISCONNECT;
            else if (strcmp(flag, "fail") == 0)
                currentNode->flags |= CLUSTER_MANAGER_FLAG_FAIL;
            else if (strcmp(flag, "slave") == 0) {
                currentNode->flags |= CLUSTER_MANAGER_FLAG_SLAVE;
                if (master_id != NULL) {
                    if (currentNode->replicate) sdsfree(currentNode->replicate);
                    currentNode->replicate = sdsnew(master_id);
                }
            }
            listAddNodeTail(currentNode->flags_str, flag);
        }
        if (config_epoch != NULL)
            currentNode->current_epoch = atoll(config_epoch);
        if (ping_sent != NULL) currentNode->ping_sent = atoll(ping_sent);
        if (ping_recv != NULL) currentNode->ping_recv = atoll(ping_recv);
        if (pang_sent != NULL) currentNode->pang_sent = atoll(pang_sent); // pang 메시지 보낸 시간 유효 시 해당 값을 currentNode의 pang_sent 멤버에 저장
        if (pang_recv != NULL) currentNode->pang_recv = atoll(pang_recv); // 응답을 받은 시간이 유효하면 해당 값을 currentNode의 pang_recv 멤버에 저장
        if (!getfriends && myself) break;
    }
cleanup:
    if (reply) freeReplyObject(reply);
    return success;
}
/*클러스터 노드 정보를 로드하는 함수에 PANG을 추가. 클러스터 노드의 정보를 가져와서 위에서 정의한 clusterManagerNode 구조체에 저장 
pang 명령어 sent와 recv에 대해 값을 채워넣고 이 값으로 클러스터 노드 간의 통신을 파악 */



//pang.json 파일 생성
{
    "PANG": {
        "summary": "Returns the server's liveliness response.",
        "complexity": "O(1)",
        "group": "connection",
        "since": "1.0.0",
        "arity": -1,
        "function": "pangCommand",
        "command_flags": [
            "FAST",
            "SENTINEL"
        ],
        "acl_categories": [
            "CONNECTION"
        ],
        "command_tips": [
            "REQUEST_POLICY:ALL_SHARDS",
            "RESPONSE_POLICY:ALL_SUCCEEDED"
        ],
        "reply_schema": {
            "anyOf": [
                {
                    "const": "PUNG",
                    "description": "Default reply."
                },
                {
                    "type": "string",
                    "description": "Relay of given `message`."
                }
            ]
        },
        "arguments": [
            {
                "name": "message",
                "type": "string",
                "optional": true
            }
        ]
    }

}
/*ping과 동일하게 pang도 json 파일을 생성하여  PANG명령어 사용법, 동작, 응답 형식을 정의*/