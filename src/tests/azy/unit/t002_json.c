#include "t002_json.h"

int
main(int argc EINA_UNUSED,
     char **argv EINA_UNUSED)
{
   int errors = 0;
   Suite *s;
   SRunner *sr;

   eina_init();
   azy_init();

   s = suite_create("azy_content_deserialize_json");
   suite_add_tcase(s, azy_content_deserialize_json_init());
   sr = srunner_create(s);

   srunner_run_all(sr, CK_NORMAL);
   errors = srunner_ntests_failed(sr);
   srunner_free(sr);

   azy_shutdown();
   eina_shutdown();
   return (errors == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
