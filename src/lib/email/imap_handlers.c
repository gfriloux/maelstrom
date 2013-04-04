#include "email_private.h"

static const char *const imap_status[] =
{
   [EMAIL_IMAP_STATUS_NONE] = "",
   [EMAIL_IMAP_STATUS_OK] = "OK",
   [EMAIL_IMAP_STATUS_NO] = "NO",
   [EMAIL_IMAP_STATUS_BAD] = "BAD"
};

static int imap_status_len[] =
{
   [EMAIL_IMAP_STATUS_NONE] = 0,
   [EMAIL_IMAP_STATUS_OK] = sizeof("OK") - 1,
   [EMAIL_IMAP_STATUS_NO] = sizeof("NO") - 1,
   [EMAIL_IMAP_STATUS_BAD] = sizeof("BAD") - 1
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
   [RESP_CODE_CAPABILITY] = CS("CAPABILITY"),
   [RESP_CODE_PARSE] = CS("PARSE"),
   [RESP_CODE_PERMANENTFLAGS] = CS("PERMANENTFLAGS"),
   [RESP_CODE_READ_ONLY] = CS("READ-ONLY"),
   [RESP_CODE_READ_WRITE] = CS("READ-WRITE"),
   [RESP_CODE_TRYCREATE] = CS("TRYCREATE"),
   [RESP_CODE_UIDNEXT] = CS("UIDNEXT"),
   [RESP_CODE_UIDVALIDITY] = CS("UIDVALIDITY"),
   [RESP_CODE_UNSEEN] = CS("UNSEEN"),
   [RESP_CODE_LAST] = CS(""),
};

typedef enum
{
   MAILBOX_FLAG_UNSUPPORTED = 0,
   MAILBOX_FLAG_HASCHILDREN,
   MAILBOX_FLAG_HASNOCHILDREN,
   MAILBOX_FLAG_MARKED,
   MAILBOX_FLAG_NOINFERIORS,
   MAILBOX_FLAG_NOSELECT,
   MAILBOX_FLAG_UNMARKED,
   MAILBOX_FLAG_LAST
} Mailbox_Flags;

static const Imap_String const MAILBOX_FLAGS[] =
{
   [MAILBOX_FLAG_UNSUPPORTED] = {"", 0},
   [MAILBOX_FLAG_HASCHILDREN] = CS("\\HASCHILDREN"),
   [MAILBOX_FLAG_HASNOCHILDREN] = CS("\\HASNOCHILDREN"),
   [MAILBOX_FLAG_MARKED] = CS("\\MARKED"),
   [MAILBOX_FLAG_NOINFERIORS] = CS("\\NOINFERIORS"),
   [MAILBOX_FLAG_NOSELECT] = CS("\\NOSELECT"),
   [MAILBOX_FLAG_UNMARKED] = CS("\\UNMARKED"),
   [MAILBOX_FLAG_LAST] = CS(""),
};

#undef CS

/* table to lookup where to start comparing cap strings based on first character
 * NULL means there's no possible matches so we can just print a fixme and skip immediately
 */
static int CAP_PREFIXES[] =
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
static int RESP_PREFIXES[] =
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
   ['m'] = 0,
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
static int MAILBOX_FLAG_PREFIXES[] =
{
   ['a'] = 0,
   ['b'] = 0,
   ['c'] = 0,
   ['d'] = 0,
   ['e'] = 0,
   ['f'] = 0,
   ['g'] = 0,
   ['h'] = MAILBOX_FLAG_HASCHILDREN,
   ['i'] = 0,
   ['j'] = 0,
   ['k'] = 0,
   ['l'] = 0,
   ['m'] = MAILBOX_FLAG_MARKED,
   ['n'] = MAILBOX_FLAG_NOINFERIORS,
   ['o'] = 0,
   ['p'] = 0,
   ['q'] = 0,
   ['r'] = 0,
   ['s'] = 0,
   ['t'] = 0,
   ['u'] = MAILBOX_FLAG_UNMARKED,
   ['v'] = 0,
   ['w'] = 0,
   ['x'] = 0,
   ['y'] = 0,
   ['z'] = 0,
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

static void
imap_dispatch_reset(Email *e)
{
   e->protocol.imap.state = e->protocol.imap.status = 0;
}


static void
next_imap(Email *e)
{
   Email_Operation *op;

   op = email_op_pop(e);
   if (!op) return;
   switch (e->current)
     {
      case EMAIL_IMAP_OP_LIST:
      case EMAIL_IMAP_OP_SELECT:
        email_imap_write(e, op, op->opdata, 0);
        break;
      default:
        break;
     }
}

static void
imap_dispatch(Email *e)
{
   Eina_Bool tofree = EINA_TRUE;

   switch (e->current)
     {
      case EMAIL_IMAP_OP_CAPABILITY: //only called during login
      case EMAIL_IMAP_OP_LOGIN:
        email_login_imap(e, NULL, 0, NULL);
        return;
      case EMAIL_IMAP_OP_LIST:
      {
        Email_List_Cb cb;
        Email_Operation *op;

        op = eina_list_data_get(e->ops);
        cb = op->cb;
        INF("LIST returned %u mboxes", eina_list_count(op->opdata));
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
imap_mailbox_flag_set(Email *e, Mailbox_Flags flag)
{
   Email_List_Item_Imap4 *it = eina_list_last_data_get(e->ev);

   it->flags |= 1 << (flag - 1);
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
             flag = prefixes[c];
             continue;
          }
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
   const unsigned char *pp, *p = NULL;

   while (1)
     {
        switch (e->protocol.imap.state)
          {
             default:
               /* state <= 0 means we have rejected any attempts to parse */
               if (size < sizeof("CAPABILITY ")) return EMAIL_RETURN_EAGAIN;
               p = data; //verified already
               if (strncasecmp((char*)p, "CAPABILITY ", sizeof("CAPABILITY ") - 1)) return EMAIL_RETURN_ERROR;
               p += sizeof("CAPABILITY ") - 1;
               e->protocol.imap.state = 1;
             case 1:
               /* actively parsing CAPS now */
               if (!p) p = data;
               if (!IMAP_FLAG_MAPPER(0, imap_cap_set, CAP_PREFIXES, CAPS, CAP_LAST)) goto out;
               /* cap could not be matched or end of caps */
               for (pp = p; DIFF(pp - data) < size - 1; pp++)
                 if (imap_atom_special(pp[0])) break;
               if (DIFF(pp - data) + 1 == size) goto out;
               p = pp;
               if (p[0] == ' ') // more caps a-comin
                 p++;
               if (isalnum(p[0])) continue; //another cap
               if (p[0] == ']') p++;
               e->protocol.imap.state = 0;
               e->need_crlf = 1;
               if (e->protocol.imap.resp)
                 {
                    /* part of respcode */
                    e->protocol.imap.caps = 1;
                    e->current = 0;
                 }
               imap_offset_update(offset, p - data);
               return EMAIL_RETURN_DONE;
             case 3:
               if (!p) p = data;
               e->protocol.imap.state = 0;
               /* more caps */
               if (p[0] == '*') continue;
               /* caps done! */
               e->protocol.imap.caps = 1;
               imap_offset_update(offset, p - data);
               return EMAIL_RETURN_DONE;
          }
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

   while (1)
     {
        switch (e->protocol.imap.state)
          {
             Email_List_Item_Imap4 *it;
             default:
               /* state <= 0 means we have rejected any attempts to parse */
               if (size < sizeof("LIST ") + 4) return EMAIL_RETURN_EAGAIN;
               p = data; //verified already
               if (strncasecmp((char*)p, "LIST ", sizeof("LIST ") - 1)) return EMAIL_RETURN_ERROR;
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
               if (!IMAP_FLAG_MAPPER(1, imap_mailbox_flag_set, MAILBOX_FLAG_PREFIXES, MAILBOX_FLAGS, MAILBOX_FLAG_LAST)) goto out;
               /* flag could not be matched or end of flags */
               for (pp = p; DIFF(pp - data) + 1< size; pp++)
                 if (imap_atom_special(pp[0]) && (pp[0] != '\\')) break; // backslash is valid here
               if (DIFF(pp - data) + 1== size) goto out;
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
               e->protocol.imap.state = 0;
               imap_offset_update(offset, pp - data);
               return EMAIL_RETURN_DONE;
          }
     }
out:
   imap_offset_update(offset, p - data);
   return EMAIL_RETURN_EAGAIN;
}

int
imap_parse_spontaneous(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   return 0;
}

int
imap_parse_respcode(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   const unsigned char *p;
   Resp_Codes code;
   unsigned char c;
/*

[CAPABILITY IMAP4rev1 UIDPLUS CHILDREN NAMESPACE THREAD=ORDEREDSUBJECT THREAD=REFERENCES SORT QUOTA IDLE AUTH=CRAM-MD5 AUTH=PLAIN ACL ACL2=UNION] Courier-IMAP ready. Copyright 1998-2011 Double Precision, Inc.  See COPYING for distribution information.\r\n

*/

   p = data;
   if (p[0] == '[') p++; //mandated by spec, but some imap servers probably suck; FIXME: examine more closely
   c = tolower(p[0]);
   for (code = RESP_PREFIXES[c]; code && (code < RESP_CODE_LAST) && (tolower(RESP_CODES[code].string[0]) == c); code++)
     {
        unsigned int len = RESP_CODES[code].len;

        if (len + 1 + DIFF(p - data) > size)
          {
             imap_offset_update(offset, p - data);
             return EMAIL_RETURN_EAGAIN;
          }
        if (!strncasecmp((char*)p, RESP_CODES[code].string, len))
          {
             imap_offset_update(offset, p - data);
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
                FIXME(PERMANENTFLAGS);
                FIXME(READ_ONLY);
                FIXME(READ_WRITE);
                FIXME(TRYCREATE);
                FIXME(UIDNEXT);
                FIXME(UIDVALIDITY);
                FIXME(UNSEEN);
                default: break;
               }
             p += len;
             break;
#undef FIXME
          }
     }
   imap_offset_update(offset, p - data);
   if (code == RESP_CODE_LAST) return EMAIL_RETURN_EAGAIN;
   e->need_crlf = 1;
   imap_dispatch_reset(e);
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
        switch (e->protocol.imap.state)
          {
           int type;
           const unsigned char *pp = data;
           size_t len = size;

           case -1: //just starting
             if (*offset)
               {
                  pp = data - 2, len += 2;
                  if (isdigit(pp[0]))
                    {
                       /* normal resp code */
                       while (isdigit(pp[-1]) && (len - *offset != size))
                         pp--, len++;
                    }
                  else
                    {
                       if (pp[0] != '*') return EMAIL_RETURN_ERROR;
                    }
               }
             type = imap_op_ok(pp, len, &opcode, NULL);
             switch (type)
               {
                case -1: return EMAIL_RETURN_ERROR; //ERROR
                case 0: return EMAIL_RETURN_EAGAIN; //EAGAIN
                case 2:
                  if (e->protocol.imap.current != opcode)
                    {
                       Eina_List *l;

                       l = imap_op_find(e, opcode);
                       if (l)
                         e->ops = eina_list_promote_list(e->ops, l);
                       else
                       {/* FFFFFFFFFFFFFFFFFFUUUUUUUUUUUUUUUUUUUUUUUUUUUU WHAT THE FUCK */}
                    }
                case 1:
                  if (size < 6) return EMAIL_RETURN_EAGAIN;
                  e->protocol.imap.resp = 1;
                  for (x = EMAIL_IMAP_STATUS_OK; x < EMAIL_IMAP_STATUS_LAST; x++)
                    if (!strncasecmp((char*)p, imap_status[x], imap_status_len[x])) break;
                  if (x == EMAIL_IMAP_STATUS_LAST) return EMAIL_RETURN_ERROR;
                  e->protocol.imap.status = x;
                  if (type == 1)
                    e->protocol.imap.state = -4;
                  else
                    {
                       e->protocol.imap.state--;
                       e->need_crlf = 1;
                    }
                  p += imap_status_len[x] + 1;
                  imap_offset_update(offset, p - data);
                  if (type == 1) continue;
               }
           case -2: //got status code, parsed CRLF
             imap_dispatch(e);
             imap_dispatch_reset(e);
             return EMAIL_RETURN_EAGAIN;
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
        p = memchr(pp, '\r', size - (pp - data));
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
        imap_offset_update(offset, p - data + 2);
        e->need_crlf = 0;
        e->protocol.imap.resp = 0;
        if (size >= DIFF(p - data) + 2)
          {
             /* check whether we're now on resp */
             if (imap_op_ok(p + 2, size - DIFF(p - data) + 2, NULL, offset) == 2)
               e->protocol.imap.state = -1;
          }
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
   switch (e->current)
     {
        case EMAIL_IMAP_OP_CAPABILITY:
          return imap_parse_caps(e, data, size, offset);
        case EMAIL_IMAP_OP_LOGIN:
          return email_login_imap(e, data, size, offset);
        case EMAIL_IMAP_OP_LIST:
          return imap_parse_list(e, data, size, offset);
        default:
          break;
     }
   return EMAIL_RETURN_EAGAIN;
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
          if (e->protocol.imap.current != opcode)
            {
               Eina_List *l;

               l = imap_op_find(e, opcode);
               if (l)
                 e->ops = eina_list_promote_list(e->ops, l);
               else
               {/* FFFFFFFFFFFFFFFFFFUUUUUUUUUUUUUUUUUUUUUUUUUUUU WHAT THE FUCK */}
            }
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
