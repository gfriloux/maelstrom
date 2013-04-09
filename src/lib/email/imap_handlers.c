#include "email_private.h"
#include <assert.h>

static const char *const imap_status[] =
{
   [EMAIL_OPERATION_STATUS_NONE] = "",
   [EMAIL_OPERATION_STATUS_OK] = "OK",
   [EMAIL_OPERATION_STATUS_NO] = "NO",
   [EMAIL_OPERATION_STATUS_BAD] = "BAD"
};

static int imap_status_len[] =
{
   [EMAIL_OPERATION_STATUS_NONE] = 0,
   [EMAIL_OPERATION_STATUS_OK] = sizeof("OK") - 1,
   [EMAIL_OPERATION_STATUS_NO] = sizeof("NO") - 1,
   [EMAIL_OPERATION_STATUS_BAD] = sizeof("BAD") - 1
};

typedef enum
{
   CAP_UNSUPPORTED = 0,
   CAP_ACL,
   CAP_AUTH_CRAM_MD5,
   CAP_AUTH_GSSAPI,
   CAP_AUTH_NTLM,
   CAP_AUTH_PLAIN,
   CAP_CHILDREN,
   CAP_IDLE,
   CAP_IMAP4,
   CAP_IMAP4rev1,
   CAP_LITERALPLUS,
   CAP_LOGINDISABLED,
   CAP_NAMESPACE,
   CAP_QUOTA,
   CAP_SORT,
   CAP_UIDPLUS,
   CAP_LAST
} Caps;

typedef struct Imap_String
{
   const char *string;
   unsigned int len;
} Imap_String;

#define CS(X) { .string = X, .len = sizeof(X) - 1 }

static const Imap_String const CAPS[] =
{
   [CAP_UNSUPPORTED] = {"", 0},
   [CAP_ACL] = CS("ACL"),
   [CAP_AUTH_CRAM_MD5] = CS("AUTH=CRAM-MD5"),
   [CAP_AUTH_GSSAPI] = CS("AUTH=GSSAPI"),
   [CAP_AUTH_NTLM] = CS("AUTH=NTLM"),
   [CAP_AUTH_PLAIN] = CS("AUTH=PLAIN"),
   [CAP_CHILDREN] = CS("CHILDREN"),
   [CAP_IDLE] = CS("IDLE"),
   [CAP_IMAP4] = CS("IMAP4"),
   [CAP_IMAP4rev1] = CS("IMAP4rev1"),
   [CAP_LITERALPLUS] = CS("LITERAL+"),
   [CAP_LOGINDISABLED] = CS("LOGINDISABLED"),
   [CAP_NAMESPACE] = CS("NAMESPACE"),
   [CAP_QUOTA] = CS("QUOTA"),
   [CAP_SORT] = CS("SORT"),
   [CAP_UIDPLUS] = CS("UIDPLUS"),
   [CAP_LAST] = CS("UIDPLUS"),
};

typedef enum
{
   RESP_CODE_UNSUPPORTED,
   RESP_CODE_ALERT,
   RESP_CODE_BADCHARSET,
   RESP_CODE_CAPABILITY,
   RESP_CODE_MYRIGHTS,
   RESP_CODE_PARSE,
   RESP_CODE_PERMANENTFLAGS,
   RESP_CODE_READ_ONLY,
   RESP_CODE_READ_WRITE,
   RESP_CODE_TRYCREATE,
   RESP_CODE_UIDNEXT,
   RESP_CODE_UIDVALIDITY,
   RESP_CODE_UNSEEN,
   RESP_CODE_LAST,
} Resp_Codes;

static const Imap_String const RESP_CODES[] =
{
   [RESP_CODE_UNSUPPORTED] = {"", 0},
   [RESP_CODE_ALERT] = CS("ALERT"),
   [RESP_CODE_BADCHARSET] = CS("BADCHARSET"),
   [RESP_CODE_CAPABILITY] = CS("CAPABILITY "),
   [RESP_CODE_MYRIGHTS] = CS("MYRIGHTS \""),
   [RESP_CODE_PARSE] = CS("PARSE"),
   [RESP_CODE_PERMANENTFLAGS] = CS("PERMANENTFLAGS ("),
   [RESP_CODE_READ_ONLY] = CS("READ-ONLY"),
   [RESP_CODE_READ_WRITE] = CS("READ-WRITE"),
   [RESP_CODE_TRYCREATE] = CS("TRYCREATE"),
   [RESP_CODE_UIDNEXT] = CS("UIDNEXT "),
   [RESP_CODE_UIDVALIDITY] = CS("UIDVALIDITY "),
   [RESP_CODE_UNSEEN] = CS("UNSEEN "),
   [RESP_CODE_LAST] = CS(""),
};

typedef enum
{
   MAILBOX_ATTRIBUTE_UNSUPPORTED = 0,
   MAILBOX_ATTRIBUTE_HASCHILDREN,
   MAILBOX_ATTRIBUTE_HASNOCHILDREN,
   MAILBOX_ATTRIBUTE_MARKED,
   MAILBOX_ATTRIBUTE_NOINFERIORS,
   MAILBOX_ATTRIBUTE_NOSELECT,
   MAILBOX_ATTRIBUTE_UNMARKED,
   MAILBOX_ATTRIBUTE_LAST
} Mailbox_Attributes;

static const Imap_String const MAILBOX_ATTRIBUTES[] =
{
   [MAILBOX_ATTRIBUTE_UNSUPPORTED] = {"", 0},
   [MAILBOX_ATTRIBUTE_HASCHILDREN] = CS("\\HASCHILDREN"),
   [MAILBOX_ATTRIBUTE_HASNOCHILDREN] = CS("\\HASNOCHILDREN"),
   [MAILBOX_ATTRIBUTE_MARKED] = CS("\\MARKED"),
   [MAILBOX_ATTRIBUTE_NOINFERIORS] = CS("\\NOINFERIORS"),
   [MAILBOX_ATTRIBUTE_NOSELECT] = CS("\\NOSELECT"),
   [MAILBOX_ATTRIBUTE_UNMARKED] = CS("\\UNMARKED"),
   [MAILBOX_ATTRIBUTE_LAST] = CS(""),
};

typedef enum
{
   MAILBOX_RIGHT_UNSUPPORTED = 0,
   MAILBOX_RIGHT_LOOKUP,
   MAILBOX_RIGHT_READ,
   MAILBOX_RIGHT_SEEN,
   MAILBOX_RIGHT_WRITE,
   MAILBOX_RIGHT_INSERT,
   MAILBOX_RIGHT_POST,
   MAILBOX_RIGHT_CREATE,
   MAILBOX_RIGHT_DELETE_MBOX,
   MAILBOX_RIGHT_DELETE_MSG,
   MAILBOX_RIGHT_EXPUNGE,
   MAILBOX_RIGHT_ADMIN,
   MAILBOX_RIGHT_OBSOLETE_DELETE, //special case
   MAILBOX_RIGHT_LAST,
} Mailbox_Rights;

typedef enum
{
   MAILBOX_FLAG_UNSUPPORTED = 0,
   MAILBOX_FLAG_ANSWERED,
   MAILBOX_FLAG_DELETED,
   MAILBOX_FLAG_DRAFT,
   MAILBOX_FLAG_FLAGGED,
   MAILBOX_FLAG_RECENT,
   MAILBOX_FLAG_SEEN,
   MAILBOX_FLAG_STAR,
   MAILBOX_FLAG_LAST
} Mailbox_Flags;

static const Imap_String const MAILBOX_FLAGS[] =
{
   [MAILBOX_FLAG_UNSUPPORTED] = CS(""),
   [MAILBOX_FLAG_ANSWERED] = CS("\\ANSWERED"),
   [MAILBOX_FLAG_DELETED] = CS("\\DELETED"),
   [MAILBOX_FLAG_DRAFT] = CS("\\DRAFT"),
   [MAILBOX_FLAG_FLAGGED] = CS("\\FLAGGED"),
   [MAILBOX_FLAG_RECENT] = CS("\\RECENT"),
   [MAILBOX_FLAG_SEEN] = CS("\\SEEN"),
   [MAILBOX_FLAG_STAR] = CS("\\*"),
   [MAILBOX_FLAG_LAST] = CS(""),
};

#undef CS

/* table to lookup where to start comparing cap strings based on first character
 * NULL means there's no possible matches so we can just print a fixme and skip immediately
 */
static int CAP_PREFIXES[128] =
{
   ['a'] = CAP_ACL,
   ['b'] = 0,
   ['c'] = CAP_CHILDREN,
   ['d'] = 0,
   ['e'] = 0,
   ['f'] = 0,
   ['g'] = 0,
   ['h'] = 0,
   ['i'] = CAP_IDLE,
   ['j'] = 0,
   ['k'] = 0,
   ['l'] = CAP_LITERALPLUS,
   ['m'] = 0,
   ['n'] = CAP_NAMESPACE,
   ['o'] = 0,
   ['p'] = 0,
   ['q'] = CAP_QUOTA,
   ['r'] = 0,
   ['s'] = CAP_SORT,
   ['t'] = 0,
   ['u'] = CAP_UIDPLUS,
   ['v'] = 0,
   ['w'] = 0,
   ['x'] = 0, //TODO: support Xtensions
   ['y'] = 0,
   ['z'] = 0,
};

/* table to lookup where to start comparing resp strings based on first character
 * NULL means there's no possible matches so we can just print a fixme and skip immediately
 */
static int RESP_PREFIXES[128] =
{
   ['a'] = RESP_CODE_ALERT,
   ['b'] = RESP_CODE_BADCHARSET,
   ['c'] = RESP_CODE_CAPABILITY,
   ['d'] = 0,
   ['e'] = 0,
   ['f'] = 0,
   ['g'] = 0,
   ['h'] = 0,
   ['i'] = 0,
   ['j'] = 0,
   ['k'] = 0,
   ['l'] = 0,
   ['m'] = RESP_CODE_MYRIGHTS,
   ['n'] = 0,
   ['o'] = 0,
   ['p'] = RESP_CODE_PARSE,
   ['q'] = 0,
   ['r'] = RESP_CODE_READ_ONLY,
   ['s'] = 0,
   ['t'] = RESP_CODE_TRYCREATE,
   ['u'] = RESP_CODE_UIDNEXT,
   ['v'] = 0,
   ['w'] = 0,
   ['x'] = 0,
   ['y'] = 0,
   ['z'] = 0,
};

/* table to lookup where to start comparing mbox attribute strings based on first character
 * NULL means there's no possible matches so we can just print a fixme and skip immediately
 */
static int MAILBOX_ATTRIBUTE_PREFIXES[128] =
{
   ['a'] = 0,
   ['b'] = 0,
   ['c'] = 0,
   ['d'] = 0,
   ['e'] = 0,
   ['f'] = 0,
   ['g'] = 0,
   ['h'] = MAILBOX_ATTRIBUTE_HASCHILDREN,
   ['i'] = 0,
   ['j'] = 0,
   ['k'] = 0,
   ['l'] = 0,
   ['m'] = MAILBOX_ATTRIBUTE_MARKED,
   ['n'] = MAILBOX_ATTRIBUTE_NOINFERIORS,
   ['o'] = 0,
   ['p'] = 0,
   ['q'] = 0,
   ['r'] = 0,
   ['s'] = 0,
   ['t'] = 0,
   ['u'] = MAILBOX_ATTRIBUTE_UNMARKED,
   ['v'] = 0,
   ['w'] = 0,
   ['x'] = 0,
   ['y'] = 0,
   ['z'] = 0,
};

/* table to lookup where to start comparing mbox flag strings based on first character
 * NULL means there's no possible matches so we can just print a fixme and skip immediately
 */
static int MAILBOX_FLAG_PREFIXES[128] =
{
   ['a'] = MAILBOX_FLAG_ANSWERED,
   ['b'] = 0,
   ['c'] = 0,
   ['d'] = MAILBOX_FLAG_DELETED,
   ['e'] = 0,
   ['f'] = MAILBOX_FLAG_FLAGGED,
   ['g'] = 0,
   ['h'] = 0,
   ['i'] = 0,
   ['j'] = 0,
   ['k'] = 0,
   ['l'] = 0,
   ['m'] = 0,
   ['n'] = 0,
   ['o'] = 0,
   ['p'] = 0,
   ['q'] = 0,
   ['r'] = MAILBOX_FLAG_RECENT,
   ['s'] = MAILBOX_FLAG_SEEN,
   ['t'] = 0,
   ['u'] = 0,
   ['v'] = 0,
   ['w'] = 0,
   ['x'] = 0,
   ['y'] = 0,
   ['z'] = 0,
   ['*'] = MAILBOX_FLAG_STAR,
};

static int MAILBOX_RIGHTS[128] =
{
   ['a'] = MAILBOX_RIGHT_ADMIN,
   ['b'] = 0,
   ['c'] = MAILBOX_RIGHT_CREATE, //OBSOLETE!
   ['d'] = MAILBOX_RIGHT_OBSOLETE_DELETE, //OBSOLETE!
   ['e'] = MAILBOX_RIGHT_EXPUNGE,
   ['f'] = 0,
   ['g'] = 0,
   ['h'] = 0,
   ['i'] = MAILBOX_RIGHT_INSERT,
   ['j'] = 0,
   ['k'] = MAILBOX_RIGHT_CREATE,
   ['l'] = MAILBOX_RIGHT_LOOKUP,
   ['m'] = 0,
   ['n'] = 0,
   ['o'] = 0,
   ['p'] = MAILBOX_RIGHT_POST,
   ['q'] = 0,
   ['r'] = MAILBOX_RIGHT_READ,
   ['s'] = MAILBOX_RIGHT_SEEN,
   ['t'] = MAILBOX_RIGHT_DELETE_MSG,
   ['u'] = 0,
   ['v'] = 0,
   ['w'] = MAILBOX_RIGHT_WRITE,
   ['x'] = MAILBOX_RIGHT_DELETE_MBOX,
   ['y'] = 0,
   ['z'] = 0,
   ['*'] = 0,
};

static Eina_List *
imap_op_find(Email *e, unsigned int opnum)
{
   Email_Operation *op;
   Eina_List *l;

   EINA_LIST_FOREACH(e->ops, l, op)
     if (op->opnum == opnum) return l;
   return NULL;
}

static Eina_List *
imap_op_find_continuable(const Email *e)
{
   Email_Operation *op;
   Eina_List *l;

   EINA_LIST_FOREACH(e->ops, l, op)
     if (op->allows_cont) return l;
   return NULL;
}

static Email_Operation *
imap_op_update(Email *e, unsigned int opcode, Eina_List *lop)
{
   Email_Operation *op = eina_list_data_get(lop);

   if (!lop)
     {
        lop = imap_op_find(e, opcode);
        if (!lop)
          {/* FFFFFFFFFFFFFFFFFFUUUUUUUUUUUUUUUUUUUUUUUUUUUU WHAT THE FUCK */}
     }
   if (lop)
     e->ops = eina_list_promote_list(e->ops, lop);
   if (e->ops)
     {
        op = eina_list_data_get(e->ops);
        e->current = op->optype;
     }
   return op;
}

/*
 * -1 invalid
 * 0 need more data
 * 1 valid
 * 2 valid+opcode
 */
static inline int
imap_op_ok(const unsigned char *data, size_t size, unsigned int *opcode, size_t *offset)
{
   unsigned int x;
   char buf[64];

   if (size < 3) return 0;
   if (data[1] == ' ')
     {
        if ((data[0] == '*') || (data[0] == '+'))
          {
             if (offset) imap_offset_update(offset, 2);
             return 1;
          }
     }
   for (x = 0; x < size; x++)
     {
        if (isdigit(data[x]))
          {
             buf[x] = data[x];
             continue;
          }
        if (data[x] == ' ')
          {
             buf[x] = 0;
             if (opcode) *opcode = strtoul(buf, NULL, 10);
             if (offset) imap_offset_update(offset, x + 1);
             return 2;
          }
        return -1;
     }
   return 0;
}

static inline Email_Imap4_Mailbox_Info *
imap_mbox_info_get(Email *e)
{
   if (!e->protocol.imap.mbox)
     {
        e->protocol.imap.mbox = calloc(1, sizeof(Email_Imap4_Mailbox_Info));
        e->protocol.imap.mbox->e = e;
     }
   return e->protocol.imap.mbox;
}

static void
imap_namespace_extension_free(Email_Imap4_Namespace_Extension *nse)
{
   Eina_Stringshare *s;

   if (!nse) return;
   eina_stringshare_del(nse->name);
   EINA_INARRAY_FOREACH(nse->flags, s)
     eina_stringshare_del(s);
   eina_inarray_free(nse->flags);
}

static void
imap_dispatch_reset(Email *e)
{
   e->protocol.imap.state = -1;
   e->protocol.imap.status = 0;
   e->current = 0;
}

void
imap_mbox_status_event(Email *e)
{
   ecore_event_add(EMAIL_EVENT_MAILBOX_STATUS, e->protocol.imap.mbox, NULL, NULL);
   e->protocol.imap.mbox = NULL;
}

static void
next_imap(Email *e)
{
   Email_Operation *op;
   Eina_List *l;

   if (e->protocol.imap.mbox)
     imap_mbox_status_event(e);
   op = email_op_pop(e);
   if (!op) return;
   
   EINA_LIST_FOREACH(e->ops, l, op)
     {
        if (email_is_blocked(e)) return;
        if (op->sent) continue;
        switch (op->optype)
          {
           case EMAIL_IMAP4_OP_LIST:
           case EMAIL_IMAP4_OP_LSUB:
           case EMAIL_IMAP4_OP_SELECT:
           case EMAIL_IMAP4_OP_EXAMINE:
           case EMAIL_IMAP4_OP_CREATE:
           case EMAIL_IMAP4_OP_DELETE:
           case EMAIL_IMAP4_OP_RENAME:
           case EMAIL_IMAP4_OP_SUBSCRIBE:
           case EMAIL_IMAP4_OP_UNSUBSCRIBE:
             email_imap_write(e, op, op->opdata, 0);
             free(op->opdata);
             op->opdata = NULL;
             break;
           case EMAIL_IMAP4_OP_APPEND:
           {
              Email_Message *msg = op->opdata;

              imap_func_message_write(op, msg, op->mbox, op->flags);
              msg->sending--;
              if (!msg->sending)
                {
                   msg->owner = NULL;
                   if (msg->deleted) email_message_free(msg);
                }
              free(op->mbox);
              op->mbox = NULL;
              break;
           }
#define CASE(TYPE) \
           case EMAIL_IMAP4_OP_##TYPE: \
             email_imap_write(e, op, EMAIL_IMAP4_##TYPE, sizeof(EMAIL_IMAP4_##TYPE) - 1); \
             break;
           CASE(EXPUNGE);
           CASE(NOOP);
           CASE(CLOSE);
           CASE(LOGOUT);
#undef CASE
           default:
             break;
          }
     }
}

static void
imap_dispatch(Email *e)
{
   Eina_Bool tofree = EINA_TRUE;
   const char *opname = "LIST";

   switch (e->current)
     {
      case EMAIL_IMAP4_OP_CAPABILITY: //only called during login
      case EMAIL_IMAP4_OP_LOGIN:
        email_login_imap(e, NULL, 0, NULL);
        return;
      case EMAIL_IMAP4_OP_LSUB:
        opname = "LSUB";
      case EMAIL_IMAP4_OP_LIST:
      {
        Email_List_Cb cb;
        Email_Operation *op;

        op = eina_list_data_get(e->ops);
        cb = op->cb;
        INF("%s returned %u mboxes", opname, eina_list_count(e->ev));
        if (cb && (!op->deleted)) tofree = !!cb(op, e->ev);
        if (tofree)
          {
             Email_List_Item_Imap4 *it;

             EINA_LIST_FREE(e->ev, it)
               {
                  eina_stringshare_del(it->name);
                  free(it);
               }
          }
        break;
      }
      case EMAIL_IMAP4_OP_SELECT:
      case EMAIL_IMAP4_OP_EXAMINE:
      {
         Email_Imap4_Mailbox_Info_Cb cb;
         Email_Operation *op;

         op = eina_list_data_get(e->ops);
         cb = op->cb;
        if (e->protocol.imap.status != EMAIL_OPERATION_STATUS_OK)
          eina_stringshare_replace(&e->protocol.imap.mboxname, NULL);
         if (cb && (!op->deleted)) tofree = !!cb(op, e->protocol.imap.mbox);
         if (tofree) email_imap4_mailboxinfo_free(e->protocol.imap.mbox);
         e->protocol.imap.mbox = NULL;
         break;
      }
      case EMAIL_IMAP4_OP_CLOSE:
        if (e->protocol.imap.status == EMAIL_OPERATION_STATUS_OK)
          eina_stringshare_replace(&e->protocol.imap.mboxname, NULL);
      case EMAIL_IMAP4_OP_LOGOUT:
      case EMAIL_IMAP4_OP_CREATE:
      case EMAIL_IMAP4_OP_DELETE:
      case EMAIL_IMAP4_OP_RENAME:
      case EMAIL_IMAP4_OP_SUBSCRIBE:
      case EMAIL_IMAP4_OP_UNSUBSCRIBE:
      case EMAIL_IMAP4_OP_APPEND:
      case EMAIL_IMAP4_OP_EXPUNGE:
      {
         Email_Cb cb;
         Email_Operation *op;

         op = eina_list_data_get(e->ops);
         cb = op->cb;
         if (cb && (!op->deleted)) cb(op, e->protocol.imap.status);
         if (e->current != EMAIL_IMAP4_OP_LOGOUT) break;
         if (e->protocol.imap.status != EMAIL_OPERATION_STATUS_OK) break;
         if (e->svr) ecore_con_server_del(e->svr);
         e->svr = NULL;
         break;
      }
      case EMAIL_IMAP4_OP_NAMESPACE:
        /* avoid sending this event right after login to ensure namespace data is available during event */
        ecore_event_add(EMAIL_EVENT_CONNECTED, e, (Ecore_End_Cb)email_fake_free, NULL);
        break;
      default: break;
     }
   next_imap(e);
}

static inline Eina_Bool
imap_atom_special(char c)
{
   switch (c)
     {
      case '(':
      case ')':
      case '{':
      case ' ':
      case '%':
      case '*':
      case ']':
      case '"':
      case '\\':
        return EINA_TRUE;
      default: break;
     }
   return EINA_FALSE;
}

static unsigned long long
imap_parse_num(const unsigned char *data, size_t size, size_t *length)
{
   char buf[256];
   unsigned long long x;

   *length = 0;
   for (x = 0; (x <= sizeof(buf)) && (x < size); x++)
     {
        if (x == sizeof(buf))
          {
             ERR("NUMBER TOO BIG! THE SEVER IS A LIE!");
             return 0;
          }
        if (isdigit(data[x]))
          buf[x] = data[x];
        else
          {
             char *b;

             if ((data[x] != ' ') && (data[x] != '\r') && (data[x] != ']')) return 0;
             buf[x] = 0;
             errno = 0;
             *length = x;
             x = strtoul(buf, &b, 10);
             if (errno)
               {
                  *length = 0;
                  return 0;
               }
             break;
          }
     }
   return x;
}

static void
imap_cap_set(Email *e, Caps cap)
{
   switch (cap)
     {
#define CASE(X) case CAP_##X: e->features.imap.X = 1; break
      CASE(CHILDREN);
      CASE(IDLE);
      CASE(IMAP4);
      CASE(IMAP4rev1);
      CASE(LITERALPLUS);
      CASE(LOGINDISABLED);
      CASE(AUTH_CRAM_MD5);
      CASE(AUTH_GSSAPI);
      CASE(AUTH_NTLM);
      CASE(AUTH_PLAIN);
      CASE(NAMESPACE);
      CASE(UIDPLUS);
#undef CASE
      default: break;
     }
}

static void
imap_mailbox_attribute_set(Email *e, Mailbox_Attributes flag)
{
   Email_List_Item_Imap4 *it = eina_list_last_data_get(e->ev);

   it->attributes |= 1 << (flag - 1);
}

static void
imap_mailbox_flag_set(Email *e, Mailbox_Flags flag)
{
   imap_mbox_info_get(e)->flags |= 1 << (flag - 1);
}

static void
imap_mailbox_permanentflag_set(Email *e, Mailbox_Flags flag)
{
   imap_mbox_info_get(e)->permanentflags |= 1 << (flag - 1);
}

/* utility for shortening flag mapper lines */
#define IMAP_FLAG_MAPPER(MAP_CHAR, CB, PREFIXES, STRINGS, LAST) \
  imap_flag_mapper(e, &p, size - (p - data), MAP_CHAR, CB, PREFIXES, (const Imap_String**)&STRINGS, LAST)

/* returns false if more data is needed */
static Eina_Bool
imap_flag_mapper(Email *e, const unsigned char **data, size_t size, unsigned int map_char, Email_Flag_Set_Cb cb, const int *prefixes, const Imap_String **strings, unsigned int last)
{
   unsigned int flag;
   unsigned char c, fs;
   const unsigned char *p = *data;
   const Imap_String *string;

   c = tolower(p[map_char]);
   if (c > 127) return EINA_TRUE; //overflow!
   for (flag = prefixes[c],
          string = (const Imap_String*)((char*)strings + (flag * sizeof(Imap_String))),
          fs = tolower(string->string[map_char]);
        flag && (flag < last) && (fs == c);
        string = (const Imap_String*)((char*)strings + (flag * sizeof(Imap_String))),
          fs = tolower(string->string[map_char]))
     {
        unsigned int len = string->len;

        *data = p;
        if (len + 1 + DIFF(p - *data) > size) return EINA_FALSE;
        if ((imap_atom_special(p[len]) /* check for substring */) && (!strncasecmp((char*)p, string->string, len)))
          {
             /* match! call cb and reset iterator */
             cb(e, flag);
             p += len;
             if (p[0] != ' ') break; //done!
             p++;
             c = tolower(p[map_char]);
             if (c > 127) break; //overflow!
             flag = prefixes[c];
          }
        else
          flag++;
     }
   *data = p;
   return EINA_TRUE;
}

/* returns:
 * -1) error
 *  0) EAGAIN
 *  1) parsing done
 */
static int
imap_parse_caps(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
/*
eg.
* CAPABILITY IMAP4 IMAP4rev1 LOGINDISABLED STARTTLS UIDPLUS CHILDREN IDLE NAMESPACE LITERAL+
 */
   const unsigned char *pp, *p = data;

   e->current = EMAIL_IMAP4_OP_CAPABILITY;
   while (1)
     {
        /* actively parsing CAPS now */
        if (!IMAP_FLAG_MAPPER(0, imap_cap_set, CAP_PREFIXES, CAPS, CAP_LAST)) goto out;
        /* cap could not be matched or end of caps */
        for (pp = p; DIFF(pp - data) < size - 1; pp++)
          if (imap_atom_special(pp[0])) break;
        if (DIFF(pp - data) + 1 == size) break;
        p = pp;
        if (p[0] == ' ') // more caps a-comin
          p++;
        if (isalnum(p[0])) continue; //another cap
        if (p[0] == ']') p++;
        e->need_crlf = 1;
        e->protocol.imap.caps = 1;
        imap_offset_update(offset, p - data);
        return EMAIL_RETURN_DONE;
     }
out:
   imap_offset_update(offset, p - data);
   return EMAIL_RETURN_EAGAIN;
}

int
imap_parse_list(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
/*

* LIST (\HasNoChildren) "." "INBOX.mu-size"
* LIST (\HasNoChildren) "." "INBOX.Queue"
* LIST (\HasNoChildren) "." "INBOX.test"
* LIST (\HasNoChildren) "." "INBOX.Templates"
* LIST (\HasNoChildren) "." "INBOX.Drafts"
* LIST (\HasNoChildren) "." "INBOX.Trash"
* LIST (\HasNoChildren) "." "INBOX.Sent"
* LIST (\Unmarked \HasChildren) "." "INBOX"
2 OK LIST completed

*/

   const unsigned char *pp, *p = NULL;
   const char *opname = "LIST ";

   if (e->current == EMAIL_IMAP4_OP_LSUB)
     opname = "LSUB ";
   while (1)
     {
        Email_List_Item_Imap4 *it;
        switch (e->protocol.imap.state)
          {
             default:
               /* state <= 0 means we have rejected any attempts to parse */
               if (size < sizeof("LIST ") + 4) return EMAIL_RETURN_EAGAIN;
               p = data; //verified already
               if (strncasecmp((char*)p, opname, sizeof("LIST ") - 1)) return EMAIL_RETURN_ERROR;
               //LIST and LSUB have same length, so using them interchangeably here is fine
               p += sizeof("LIST ") - 1;
               if (!e->protocol.imap.resp)
                 e->ev = eina_list_append(e->ev, calloc(1, sizeof(Email_List_Item_Imap4)));
               if (p[0] != '(') return EMAIL_RETURN_ERROR;
               if (p[1] == ')') //no flags
                 {
                    p += 3;
                    e->protocol.imap.state = 2;
                    continue;
                 }
               e->protocol.imap.state = 1;
               p++;
             case 1:
               if (!p) p = data;
               if (DIFF(p - data) + 7 >= size) goto out;
               if (!IMAP_FLAG_MAPPER(1, imap_mailbox_attribute_set, MAILBOX_ATTRIBUTE_PREFIXES, MAILBOX_ATTRIBUTES, MAILBOX_ATTRIBUTE_LAST)) goto out;
               /* flag could not be matched or end of flags */
               for (pp = p; DIFF(pp - data) + 1< size; pp++)
                 if (imap_atom_special(pp[0]) || (pp[0] == '\\')) break; // backslash is valid here
               if (DIFF(pp - data) + 1 == size) goto out;
               p = pp;
               if (p[0] == ' ') // more flags a-comin
                 p++;
               if (p[0] == '\\') continue; //another flag
               if (p[0] != ')') return EMAIL_RETURN_ERROR;
               p += 2;
               e->protocol.imap.state++;
             case 2: //hierarchy delimiter
               if (!p) p = data;
               if (DIFF(p - data) + 5 >= size) goto out;
               if (p[0] != '"')
                 {
                    if (memcmp(p, "NIL ", 4)) return EMAIL_RETURN_ERROR;
                    e->protocol.imap.state = 4;
                    p += 4;
                    continue;
                 }
               p++;
               pp = memchr(p, '"', size - (p - data));
               if (!pp) goto out;
               p = pp;
               e->protocol.imap.state++;
             case 3: //end dquote for hierarchy delimiter
               if (!p) p = data;
               if (DIFF(p - data) + 3 >= size) goto out;
               if (p[0] != '"') return EMAIL_RETURN_ERROR;
               p += 2;
               e->protocol.imap.state++;
             case 4: //mbox name (finally)
               if (!p) p = data;
               it = eina_list_last_data_get(e->ev);
               if (DIFF(p - data) + 2 > size) goto out;
               if (p[0] == '"') p++; //optional dquote allowed
               for (pp = p; (!imap_atom_special(pp[0])) && DIFF(pp - data) < size; pp++);
               if (!imap_atom_special(pp[0])) goto out;
               if ((pp - p != 5) || strncasecmp((char*)p, "INBOX", 5))
                 it->name = eina_stringshare_add_length((char*)p, pp - p);
               else
                 it->name = eina_stringshare_add("INBOX"); //ensure caps here for easier use
               DBG("MBOX: %s", it->name);
               e->need_crlf = 1;
               imap_offset_update(offset, pp - data);
               return EMAIL_RETURN_DONE;
          }
     }
out:
   imap_offset_update(offset, p - data);
   return EMAIL_RETURN_EAGAIN;
}

int
imap_parse_mailbox_flags(Email *e, Email_Flag_Set_Cb cb, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *pp, *p = data;
   int ret = EMAIL_RETURN_EAGAIN;

   while (1)
     {
        if (!IMAP_FLAG_MAPPER(1, cb, MAILBOX_FLAG_PREFIXES, MAILBOX_FLAGS, MAILBOX_FLAG_LAST)) goto out;
        /* flag could not be matched or end of flags */
        for (pp = p; DIFF(pp - data) + 1 < size; pp++)
          if (imap_atom_special(pp[0]) || (pp[0] == '\\')) break; // backslash is valid here
        if (DIFF(pp - data) + 1 == size) goto out;
        p = pp;
        if (p[0] == ' ') // more flags a-comin
          p++;
        if (p[0] == '\\') continue; //another flag
        if (p[0] != ')') return EMAIL_RETURN_ERROR;
        p++;
        e->need_crlf = 1;
        ret = EMAIL_RETURN_DONE;
        break;
     }
out:
   imap_offset_update(offset, p - data);
   return ret;
}

int
imap_parse_mailbox_rights(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *p = data;
   int ret = EMAIL_RETURN_EAGAIN;
   Mailbox_Rights flag;

   for (; DIFF(p - data) < size; p++)
     {
        if (p[0] == '"')
          {
             e->need_crlf = 1;
             ret = EMAIL_RETURN_DONE;
             break;
          }
        if (!islower(p[0])) return EMAIL_RETURN_ERROR;
        flag = MAILBOX_RIGHTS[p[0]];
        if (flag)
          {
             if (flag == MAILBOX_RIGHT_OBSOLETE_DELETE)
               {
                  imap_mbox_info_get(e)->rights |= 1 << (MAILBOX_RIGHT_DELETE_MBOX - 1);
                  imap_mbox_info_get(e)->rights |= 1 << (MAILBOX_RIGHT_DELETE_MSG - 1);
               }
             else
               imap_mbox_info_get(e)->rights |= 1 << (flag - 1);
          }
        else
          ERR("UNKNOWN MAILBOX RIGHT FLAG: %c", p[0]);
     }
   imap_offset_update(offset, p - data);
   return ret;
}

int
imap_parse_respcode_nums(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *pp, *p = data;
   unsigned int x;
   size_t len;

   /* this gets too troublesome to parse if the whole line isn't received yet */
   pp = memchr(p, '\r', size);
   if (!pp) return EMAIL_RETURN_EAGAIN;
   x = imap_parse_num(p, size, &len);
   if (!len) goto error;
   p += len;
   if (p[0] != ' ') return EMAIL_RETURN_ERROR;
   p++;
   if (!strncasecmp((char*)p, "EXISTS", sizeof("EXISTS") - 1))
     imap_mbox_info_get(e)->exists = x;
   else if (!strncasecmp((char*)p, "RECENT", sizeof("RECENT") - 1))
     imap_mbox_info_get(e)->recent = x;
   else if (!strncasecmp((char*)p, "EXPUNGE", sizeof("EXPUNGE") - 1))
     {
        if (!imap_mbox_info_get(e)->expunge)
          imap_mbox_info_get(e)->expunge = eina_inarray_new(sizeof(int), 0);
        eina_inarray_push(imap_mbox_info_get(e)->expunge, &x);
     }
   else if (!strncasecmp((char*)p, "FETCH", sizeof("FETCH") - 1))
     {
        if (!imap_mbox_info_get(e)->fetch)
          imap_mbox_info_get(e)->fetch = eina_inarray_new(sizeof(Email_Imap4_Message), 0);
        /* FIXME: rest of rfc3501 p87 msg-att-static */
        eina_inarray_push(imap_mbox_info_get(e)->fetch, &x);
     }
   else goto error;
   e->need_crlf = 1;
   imap_offset_update(offset, pp - data);
   return EMAIL_RETURN_DONE;

error:
   free(e->ev);
   e->ev = NULL;
   return EMAIL_RETURN_ERROR;
}

int
imap_parse_namespace(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *pp, *p = data;
   Email_Imap4_Namespace_Type type = EMAIL_IMAP4_NAMESPACE_PERSONAL;

/*

 (("" "/")) (("~" "/")) (("#shared/" "/")("#public/" "/")("#ftp/" "/")("#news." "."))

*/

   e->current = EMAIL_IMAP4_OP_NAMESPACE;
   for (; type < EMAIL_IMAP4_NAMESPACE_LAST; type++)
     {
        p++;
        if (p[0] != '(')
          {
             if (!memcmp(p, "NIL", 3)) //empty namespace
               {
                  p += 3;
                  continue;
               }
             //FIXME: leaks
             return EMAIL_RETURN_ERROR;
          }
        for (p++; p[0] == '('; p++)
          {
             Email_Imap4_Namespace ns = {NULL, NULL, NULL};

             p++;
             assert(p[0] == '"');
             p++;
             if (!e->features.imap.namespaces[type])
               e->features.imap.namespaces[type] = eina_inarray_new(sizeof(Email_Imap4_Namespace), 0);
             if (p[0] != '"')
               {
                  pp = p + 1;
                  while (pp[0] != '"') pp++;
                  ns.prefix = eina_stringshare_add_length((char*)p, DIFF(pp - p));
                  p = pp;
               }
             assert(p[0] == '"');
             p += 2;
             assert(p[0] == '"');
             p++;
             if (p[0] != '"')
               {
                  pp = p + 1;
                  while (pp[0] != '"') pp++;
                  ns.delim = eina_stringshare_add_length((char*)p, DIFF(pp - p));
                  p = pp;
               }
             assert(p[0] == '"');
             for (; p[0] == ' '; p++)
               {
                  Email_Imap4_Namespace_Extension *nse;
                  /* extensions :( */
                  p++;
                  assert(p[0] == '"');
                  p++;
                  assert(p[0] != '"');
                  if (!ns.extensions)
                    ns.extensions = eina_hash_string_superfast_new((Eina_Free_Cb)imap_namespace_extension_free);
                  pp = p + 1;
                  while (pp[0] != '"') pp++;
                  nse = calloc(1, sizeof(Email_Imap4_Namespace_Extension));
                  nse->name = eina_stringshare_add_length((char*)p, DIFF(pp - p));
                  p = pp + 1;
                  assert(p[0] == ' ');
                  p++;
                  assert(p[0] == '(');
                  p++;
                  nse->flags = eina_inarray_new(sizeof(char*), 0);
                  while (p[0] == '"')
                    {
                       Eina_Stringshare *flag;

                       p++;
                       assert(p[0] != '"');
                       pp = p + 1;
                       while (pp[0] != '"') pp++;
                       flag = eina_stringshare_add_length((char*)p, DIFF(pp - p));
                       eina_inarray_push(nse->flags, &flag);
                       p = pp + 1;
                       if (p[0] == ')') break;
                       p++;
                    }
                  eina_hash_direct_add(ns.extensions, nse->name, nse);
                  p++;
               }
             p++;
             eina_inarray_push(e->features.imap.namespaces[type], &ns);
          }
        p++;
     }
   e->protocol.imap.state = 0;
   e->need_crlf = 1;
   imap_offset_update(offset, p - data);
   return EMAIL_RETURN_DONE;
}

int
imap_data_enum_handle(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *p = data;
   Email_Operation *op;

   op = eina_list_data_get(e->ops);
   switch (e->current)
     {
        case EMAIL_IMAP4_OP_CAPABILITY:
          p += sizeof("CAPABILITY ") - 1;
          imap_offset_update(offset, DIFF(p - data));
          return imap_parse_caps(e, p, size - DIFF(p - data), offset);
        case EMAIL_IMAP4_OP_LOGIN:
          return email_login_imap(e, data, size, offset);
        case EMAIL_IMAP4_OP_NAMESPACE:
          p += sizeof("NAMESPACE") - 1; // parser needs an extra ' '
          imap_offset_update(offset, DIFF(p - data));
          return imap_parse_namespace(e, p, size - DIFF(p - data), offset);
        case EMAIL_IMAP4_OP_LIST:
        case EMAIL_IMAP4_OP_LSUB:
          return imap_parse_list(e, data, size, offset);
        case EMAIL_IMAP4_OP_SELECT:
        case EMAIL_IMAP4_OP_EXAMINE:
        case EMAIL_IMAP4_OP_NOOP:
        case EMAIL_IMAP4_OP_CLOSE:
        case EMAIL_IMAP4_OP_CREATE:
        case EMAIL_IMAP4_OP_DELETE:
        case EMAIL_IMAP4_OP_RENAME:
        case EMAIL_IMAP4_OP_SUBSCRIBE:
        case EMAIL_IMAP4_OP_UNSUBSCRIBE:
          e->protocol.imap.state = -1;
          return EMAIL_RETURN_DONE;
        case EMAIL_IMAP4_OP_APPEND: // received go ahead for continuation
          imap_offset_update(offset, size);
          email_write(e, ESBUF(op->opdata), ESBUFLEN(op->opdata));
          email_write(e, CRLF, CRLFLEN);
          eina_strbuf_free(op->opdata);
          op->opdata = NULL;
          e->protocol.imap.state = -1;
          break;
        case EMAIL_IMAP4_OP_LOGOUT: //going to assume our users aren't idiots here...
          e->need_crlf = 1;
          return EMAIL_RETURN_DONE;
        default:
          if (e->protocol.imap.state < EMAIL_STATE_CONNECTED)
            return email_login_imap(e, data, size, offset);
          break;
     }
   return EMAIL_RETURN_EAGAIN;
}

int
imap_parse_respcode_bracket(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *p = data;
   Resp_Codes code;
   unsigned char c;
/*

[CAPABILITY IMAP4rev1 UIDPLUS CHILDREN NAMESPACE THREAD=ORDEREDSUBJECT THREAD=REFERENCES SORT QUOTA IDLE AUTH=CRAM-MD5 AUTH=PLAIN ACL ACL2=UNION] Courier-IMAP ready. Copyright 1998-2011 Double Precision, Inc.  See COPYING for distribution information.\r\n

*/
   if (p[0] == '[') p++;
   c = tolower(p[0]);
   for (code = RESP_PREFIXES[c]; code && (code < RESP_CODE_LAST) && (tolower(RESP_CODES[code].string[0]) == c); code++)
     {
        size_t len = RESP_CODES[code].len;

        if (len + 1 + DIFF(p - data) > size)
          {
             imap_offset_update(offset, p - data);
             return EMAIL_RETURN_EAGAIN;
          }
        if (!strncasecmp((char*)p, RESP_CODES[code].string, len))
          {
             p += len;
             imap_offset_update(offset, DIFF(p - data));
             switch (code)
               {
#define FIXME(CASE) \
  case RESP_CODE_##CASE: \
    ERR("FIXME: %s", #CASE); \
    break
                FIXME(ALERT);
                FIXME(BADCHARSET);
                case RESP_CODE_CAPABILITY:
                  return imap_parse_caps(e, p, size - DIFF(p - data), offset);
                FIXME(PARSE);
                case RESP_CODE_MYRIGHTS:
                  return imap_parse_mailbox_rights(e, p, size - DIFF(p - data), offset);
                case RESP_CODE_PERMANENTFLAGS:
                  return imap_parse_mailbox_flags(e, imap_mailbox_permanentflag_set, p, size - DIFF(p - data), offset);
                case RESP_CODE_READ_ONLY:
                  imap_mbox_info_get(e)->access = EMAIL_IMAP4_MAILBOX_ACCESS_READONLY;
                  break;
                case RESP_CODE_READ_WRITE:
                  imap_mbox_info_get(e)->access = EMAIL_IMAP4_MAILBOX_ACCESS_READWRITE;
                  break;
                FIXME(TRYCREATE);
                case RESP_CODE_UIDNEXT:
                  imap_mbox_info_get(e)->uidnext = imap_parse_num(p, size - (p - data), &len);
                  break;
                case RESP_CODE_UIDVALIDITY:
                  imap_mbox_info_get(e)->uidvalidity = imap_parse_num(p, size - (p - data), &len);
                  break;
                case RESP_CODE_UNSEEN:
                  imap_mbox_info_get(e)->unseen = imap_parse_num(p, size - (p - data), &len);
                  break;
                default: break;
               }
             break;
#undef FIXME
          }
     }
   imap_offset_update(offset, p - data);
   if (code == RESP_CODE_LAST) return EMAIL_RETURN_EAGAIN;
   e->need_crlf = 1;
   return EMAIL_RETURN_DONE;
}

int
imap_parse_respcode_mboxdata(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *p = data;

   /* FIXME: optimize */
   if (!strncasecmp((char*)p, "FLAGS (", sizeof("FLAGS (") - 1))
     {
        p += sizeof("FLAGS (") - 1;
        imap_offset_update(offset, DIFF(p - data));
        return imap_parse_mailbox_flags(e, imap_mailbox_flag_set, p, size - DIFF(p - data), offset);
     }
   if (!strncasecmp((char*)p, "LIST ", 5))
     {
        e->current = EMAIL_IMAP4_OP_LIST;
        return imap_parse_list(e, data, size, offset);
     }
   if (!strncasecmp((char*)p, "NAMESPACE ", 10))
     {
        e->current = EMAIL_IMAP4_OP_NAMESPACE;
        return imap_data_enum_handle(e, data, size, offset);
     }
   if (!strncasecmp((char*)p, "CAPABILITY (", 10))
     {
        e->current = EMAIL_IMAP4_OP_CAPABILITY;
        return imap_data_enum_handle(e, data, size, offset);
     }
   if (!strncasecmp((char*)p, "LSUB (", 6))
     {
        e->current = EMAIL_IMAP4_OP_LSUB;
        return imap_parse_list(e, data, size, offset);
     }
   e->need_crlf = 1;
   return EMAIL_RETURN_DONE;
}

int
imap_parse_respcode(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *p = data;

   if (p[0] == '[') return imap_parse_respcode_bracket(e, data, size, offset);
   if (isdigit(p[0])) return imap_parse_respcode_nums(e, data, size, offset);
   if (isalpha(p[0])) return imap_parse_respcode_mboxdata(e, data, size, offset);
   e->need_crlf = 1;
   return EMAIL_RETURN_DONE;
}

int
imap_parse_resp_cond(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *p = data;
   unsigned int opcode, x;
/* looking to complete a resp line such as:

1 OK CAPABILITY completed.CRLF

*/
   while (1)
     {
        int type;
        const unsigned char *pp = data;
        size_t len = size;

        switch (e->protocol.imap.state)
          {
           case -1: //just starting
             if (*offset)
               {
                  pp = data - 2, len += 2;
                  if (isdigit(pp[0]))
                    {
                       /* normal resp code */
                       while ((len - *offset != size) && isdigit(pp[-1]))
                         pp--, len++;
                    }
                  else
                    {
                       if (pp[0] == '+')
                         {
                            if (!e->current)
                              {
                                 Eina_List *l;
                                 l = imap_op_find_continuable(e);
                                 if (l) imap_op_update(e, 0, l);
                              }
                            return imap_data_enum_handle(e, data, size, offset);
                         }
                       if (pp[0] != '*')
                         {
                            /* some fucking servers don't know how to tag logouts (courier-imap) */
                            if (e->current != EMAIL_IMAP4_OP_LOGOUT) return EMAIL_RETURN_ERROR;
                         }
                    }
               }
             type = imap_op_ok(pp, len, &opcode, NULL);
             switch (type)
               {
                case 0: return EMAIL_RETURN_EAGAIN; //EAGAIN
                case -1:
                  /* some fucking servers don't know how to tag logouts (courier-imap) */
                  if (e->current != EMAIL_IMAP4_OP_LOGOUT) return EMAIL_RETURN_ERROR;
                case 2:
                  if (type == 2) imap_op_update(e, opcode, NULL);
                  e->protocol.imap.resp = 1;
                case 1:
                  if (size < 6) return EMAIL_RETURN_EAGAIN;
                  for (x = EMAIL_OPERATION_STATUS_OK; x < EMAIL_OPERATION_STATUS_LAST; x++)
                    if (!strncasecmp((char*)p, imap_status[x], imap_status_len[x])) break;
                  if (x != EMAIL_OPERATION_STATUS_LAST) e->protocol.imap.status = x;
                  else if (type == 2) return EMAIL_RETURN_ERROR;
                  if (e->protocol.imap.status)
                    {
                       p += imap_status_len[x] + 1;
                       imap_offset_update(offset, DIFF(p - data));
                    }
                  if (type == 1)
                    {
                       e->protocol.imap.state = -4;
                       continue;
                    }
                  e->protocol.imap.state--;
                  e->need_crlf = 1;
               }
           case -2: //got status code, maybe parsed CRLF
             return e->need_crlf ? EMAIL_RETURN_DONE : EMAIL_RETURN_EAGAIN;
           case -4: // parsing respcode (some additional data)
             if (!p) p = data;
             return imap_parse_respcode(e, p, size - DIFF(p - data), offset);
          }
     }
   return EMAIL_RETURN_DONE;
}

int
imap_parse_crlf(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *p, *pp;

   pp = data;
   while (1)
     {
        p = memchr(pp, '\r', size - DIFF(pp - data));
        if ((!p) || (DIFF(p - data) + 2 > size))
          {
             imap_offset_update(offset, size);
             return EMAIL_RETURN_EAGAIN;
          }
        if (p[1] != '\n')
          {
             pp = p + 1;
             continue;
          }
        imap_offset_update(offset, DIFF(p - data) + 2);
        e->need_crlf = 0;
        if (e->protocol.imap.resp)
          {
             /* reading the response code line, data possibly finished */
             Email_Operation *op = eina_list_data_get(e->ops);

             if (op) op->opdata = strndup((char*)data, p - pp);
             imap_dispatch(e);
             imap_dispatch_reset(e);
          }
        e->protocol.imap.resp = 0;
        if (size > DIFF(p - data) + 2)
          {
             /* check whether we're now on resp */
             imap_op_ok(p + 2, size - DIFF(p - data) + 2, NULL, offset);
             e->protocol.imap.state = -1;
               
          }
        else if (e->ops && (size == DIFF(p - data) + 2)) //no more data to read for current op
          return EMAIL_RETURN_EAGAIN;
        break;
     }
   return EMAIL_RETURN_DONE;
}

int
imap_data_handle(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   if (e->need_crlf)
     return imap_parse_crlf(e, data, size, offset);
   if (e->protocol.imap.state < 0)
     /* reading a response line */
     return imap_parse_resp_cond(e, data, size, offset);
   return imap_data_enum_handle(e, data, size, offset);
}

int
imap_data_pre_handle(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   int ret;

   if (e->buf)
     ret = imap_data_handle(e, EBUF(e->buf) + *offset, EBUFLEN(e->buf) - *offset, offset);
   else
     ret = imap_data_handle(e, data + *offset, size - *offset, offset);
   switch (ret)
     {
      case -1: //invalid data
        CRI("Invalid data");
        //if (e->buf) eina_binbuf_free(e->buf);
        //e->buf = NULL;
        /* FIXME: not sure what to do here...skip to next CRLF maybe */
        return EMAIL_RETURN_ERROR;
      case 0: //EAGAIN
        return EMAIL_RETURN_EAGAIN;
      case 1: //current parser done
        break;
     }
   return EMAIL_RETURN_DONE;
}

Eina_Bool
data_imap(Email *e, int type EINA_UNUSED, Ecore_Con_Event_Server_Data *ev)
{
   unsigned int opcode;
   int ret;
   const unsigned char *data;
   size_t size, offset = 0;

   if (e != ecore_con_server_data_get(ev->server))
     {
        DBG("Event mismatch");
        return ECORE_CALLBACK_PASS_ON;
     }

   if (eina_log_domain_level_check(email_log_dom, EINA_LOG_LEVEL_DBG))
     {
        char *recvbuf;
        recvbuf = alloca(ev->size + 1);
        memcpy(recvbuf, ev->data, ev->size);
        recvbuf[ev->size] = 0;
        DBG("Receiving %i bytes:\n%s", ev->size, recvbuf);
     }

   if (e->buf)
     eina_binbuf_append_length(e->buf, ev->data, ev->size);
   if (memcmp(ev->data + ev->size - 2, CRLF, CRLFLEN))
     {
        DBG("Deferring obviously incomplete response");
        if (!e->buf)
          {
             e->buf = eina_binbuf_manage_new_length(ev->data, ev->size);
             ev->data = NULL;
          }
       return ECORE_CALLBACK_RENEW;
     }
   data = e->buf ? EBUF(e->buf) : ev->data;
   size = e->buf ? EBUFLEN(e->buf) : ((size_t)ev->size);
   switch (imap_op_ok(data, size, &opcode, &offset))
     {
        case -1:
          if (!e->protocol.imap.state)
            {
                //if we haven't engaged a parser and the data is invalid, bail here
               ecore_con_server_del(ev->server);
               ERR("probably not an imap server?");
            }
          return ECORE_CALLBACK_RENEW;
        case 0:
          if (!e->buf)
            {
               e->buf = eina_binbuf_manage_new_length(ev->data, ev->size);
               ev->data = NULL;
            }
          return ECORE_CALLBACK_RENEW;
        case 2:
          imap_op_update(e, opcode, NULL);
        case 1:
          do
          {
             ret = imap_data_pre_handle(e, data, size, &offset);
          } while ((ret == EMAIL_RETURN_DONE) && (offset < size));
          if (e->buf)
            {
               if (offset == size)
                 {
                    eina_binbuf_free(e->buf);
                    e->buf = NULL;
                 }
               else
                 eina_binbuf_remove(e->buf, 0, offset);
            }
          else if (offset < size)
            {
               if (offset)
                 {
                    e->buf = eina_binbuf_new();
                    eina_binbuf_append_length(e->buf, data + offset, size - offset);
                 }
               else
                 {
                    e->buf = eina_binbuf_manage_new_length(ev->data, size);
                    ev->data = NULL;
                 }
            }
          data = e->buf ? EBUF(e->buf) : NULL;
          size = e->buf ? EBUFLEN(e->buf) : 0;
          switch (ret)
            {
             case 1: break;
             case EMAIL_RETURN_EAGAIN: return ECORE_CALLBACK_RENEW;
             case EMAIL_RETURN_ERROR:
             default:
               /* errors should be handled better than this... */
               return ECORE_CALLBACK_RENEW;
            }
          break;
     }
   if (e->state < EMAIL_STATE_CONNECTED)
     {
        email_login_imap(e, data, size, NULL);
        return ECORE_CALLBACK_RENEW;
     }

   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
error_imap(Email *e EINA_UNUSED, int type EINA_UNUSED, Ecore_Con_Event_Server_Error *ev EINA_UNUSED)
{
   ERR("Error");
   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
upgrade_imap(Email *e, int type EINA_UNUSED, Ecore_Con_Event_Server_Upgrade *ev)
{
   if (e != ecore_con_server_data_get(ev->server)) return ECORE_CALLBACK_PASS_ON;

   e->state++;
   email_login_imap(e, NULL, 0, NULL);
   return ECORE_CALLBACK_RENEW;
}
