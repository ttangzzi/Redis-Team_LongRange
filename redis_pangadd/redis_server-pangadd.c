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