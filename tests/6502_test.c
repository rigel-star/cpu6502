#include "./6502.h"
#include <check.h>
#include <stdlib.h>

START_TEST(add_byte_check)
{
	int ans = add_ints(3, 4);	
	ck_assert_int_eq(ans, 8);
}
END_TEST

Suite *byte_operations_suite(void) {       
  Suite *s;                      
  TCase *tc_core;                

  s = suite_create("Bytes");     
  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, add_byte_check); 
  suite_add_tcase(s, tc_core);                
  return s;
}

int main(void) {
  int no_failed = 0;                   
  Suite *s;                            
  SRunner *runner;                     

  s = byte_operations_suite();                   
  runner = srunner_create(s);          

  srunner_run_all(runner, CK_NORMAL);  
  no_failed = srunner_ntests_failed(runner); 
  srunner_free(runner);                      
  return (no_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;  
}
