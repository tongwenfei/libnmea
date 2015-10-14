#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nmea.h>

#include "minunit.h"

int tests_run = 0;

static char *
test_get_type_ok()
{
	const char *sentence;
  nmea_t res;

  sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A,*1D\n\n");
  res = nmea_get_type(sentence);
  mu_assert("nmea_get_type() should return correct type (GPGLL)", NMEA_GPGLL == res);

	sentence = strdup("$GPGGA,4916.45,N,12311.12,W,225444,A\n\n");
  res = nmea_get_type(sentence);
  mu_assert("nmea_get_type() should return correct type (GPGGA)", NMEA_GPGGA == res);

  return 0;
}

static char *
test_get_type_unknown()
{
	const char *sentence;
  nmea_t res;

  sentence = strdup("THISISWRONG");
  res = nmea_get_type(sentence);
  mu_assert("nmea_get_type() should return NMEA_UNKNOWN on unknown sentence type", NMEA_UNKNOWN == res);

  sentence = strdup("$UNKNOWN");
  res = nmea_get_type(sentence);
  mu_assert("nmea_get_type() should return NMEA_UNKNOWN on unknown sentence type", NMEA_UNKNOWN == res);

  sentence = strdup("");
  res = nmea_get_type(sentence);
  mu_assert("nmea_get_type() should return nmea_unknown on empty sentence", NMEA_UNKNOWN == res);

  return 0;
}

static char *
test_get_checksum_with_crc()
{
	// Sentence with checksum (0x1D == 29)
	const char *sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A,*1D\n\n");
  uint8_t res = nmea_get_checksum(sentence);
  mu_assert("nmea_get_checksum() should return correct checksum", 29 == res);

  return 0;
}

static char *
test_get_checksum_without_crc()
{
	// Sentence without checksum (0x1D == 29)
	const char *sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A,\n\n");
  uint8_t res = nmea_get_checksum(sentence);
  mu_assert("nmea_get_checksum() should return correct checksum", 29 == res);

  return 0;
}

static char *
test_get_checksum_too_long_sentence()
{
	// Sentence without correct ending (ex: \r\n)
	const char *sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A,");
  uint8_t res = nmea_get_checksum(sentence);
  mu_assert("nmea_get_checksum() should return 0 when sentence is too long", 0 == res);

  return 0;
}

static char *
test_has_checksum_yes()
{
	// Sentence with checksum
	const char *sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A*1D\n\n");
  int res = nmea_has_checksum(sentence, strlen(sentence));
  mu_assert("nmea_has_checksum() should return 0 when sentence has a checksum", 0 == res);

  return 0;
}

static char *
test_has_checksum_no()
{
	// Sentence without checksum
	const char *sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A\n\n");
  int res = nmea_has_checksum(sentence, strlen(sentence));
  mu_assert("nmea_has_checksum() should return -1 when sentence does not have a checksum", -1 == res);

  return 0;
}

static char *
test_validate_ok_with_crc()
{
	// Valid sentence with checksum
	const char *sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A,*1D\n\n");
  int res = nmea_validate(sentence, strlen(sentence), 1);
  mu_assert("nmea_validate() should return 0 when sentence is valid", 0 == res);

  return 0;
}

static char *
test_validate_ok_without_crc()
{
  const char *sentence;
  int res;

	// Valid sentence without checksum
	sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A\n\n");
  res = nmea_validate(sentence, strlen(sentence), 1);
  mu_assert("nmea_validate() should return 0 when sentence is valid", 0 == res);

	// Valid sentence with invalid checksum
	sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A*FF\n\n");
  res = nmea_validate(sentence, strlen(sentence), 0);
  mu_assert("nmea_validate() should return 0 when check_checksum is 0 and crc is invalid", 0 == res);

  return 0;
}

static char *
test_validate_fail_type()
{
	// Invalid sentence type
	const char *sentence = strdup("$GPgll,4916.45,N,12311.12,W,225444,A\n\n");
  int res = nmea_validate(sentence, strlen(sentence), 1);
  mu_assert("nmea_validate() should return -1 when sentence type is invalid", -1 == res);

  return 0;
}

static char *
test_validate_fail_start()
{
	// Invalid sentence start (no $ sign)
	const char *sentence = strdup("£GPGLL,4916.45,N,12311.12,W,225444,A\n\n");
  int res = nmea_validate(sentence, strlen(sentence), 1);
  mu_assert("nmea_validate() should return -1 when sentence start is invalid", -1 == res);

  return 0;
}

static char *
test_validate_fail_end()
{
  const char *sentence;
  int res;

	// Invalid sentence ending (no \n)
	sentence = strdup("$GPGLL,4916.45,N,12311.12,W,225444,A");
  res = nmea_validate(sentence, strlen(sentence), 1);
  mu_assert("nmea_validate() should return -1 when sentence ending is invalid", -1 == res);

	// Too short sentence
	sentence = strdup("$");
  res = nmea_validate(sentence, strlen(sentence), 1);
  mu_assert("nmea_validate() should return -1 when sentence is too short", -1 == res);

  return 0;
}

static char *
test_validate_fail_empty()
{
	// Invalid sentence (empty string)
	const char *sentence = strdup("");
  int res = nmea_validate(sentence, strlen(sentence), 1);
  mu_assert("nmea_validate() should return -1 when sentence is empty", -1 == res);

  return 0;
}

static char *
test_parse_ok()
{
	char *sentence;
  nmea_s *res;

  // With crc
  sentence = strdup("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n\n");
  res = nmea_parse(sentence, strlen(sentence), 1);
  mu_assert("nmea_parse() should be able to parse a GPGGA sentence", NULL != res);

  // With invalid crc, but check disabled
  sentence = strdup("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*FF\n\n");
  res = nmea_parse(sentence, strlen(sentence), 0);
  mu_assert("nmea_parse() should be able to parse a GPGGA sentence", NULL != res);

  return 0;
}

static char *
test_parse_unknown()
{
	char *sentence;
  nmea_s *res;

  sentence = strdup("$JACK1,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n\n");
  res = nmea_parse(sentence, strlen(sentence), 1);
  mu_assert("nmea_parse() should return NULL when sentence type is unknown", NULL == res);

  return 0;
}

static char *
test_parse_invalid()
{
	char *sentence;
  nmea_s *res;

  sentence = strdup("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*FF\n\n");
  res = nmea_parse(sentence, strlen(sentence), 1);
  mu_assert("nmea_parse() should return NULL when checksum is invalid", NULL != res);

  sentence = strdup("");
  res = nmea_parse(sentence, strlen(sentence), 1);
  mu_assert("nmea_parse() should return NULL when sentence is empty", NULL == res);

  sentence = strdup("invalid");
  res = nmea_parse(sentence, strlen(sentence), 1);
  mu_assert("nmea_parse() should return NULL when sentence is invalid", NULL == res);

  res = nmea_parse(NULL, 0, 1);
  mu_assert("nmea_parse() should return NULL when sentence is NULL", NULL == res);

  return 0;
}

static char *
all_tests()
{
  mu_run_test(test_get_type_ok);
  mu_run_test(test_get_type_unknown);

  mu_run_test(test_get_checksum_with_crc);
  mu_run_test(test_get_checksum_without_crc);
  mu_run_test(test_get_checksum_too_long_sentence);

  mu_run_test(test_has_checksum_yes);
  mu_run_test(test_has_checksum_no);

  mu_run_test(test_validate_ok_with_crc);
  mu_run_test(test_validate_ok_without_crc);

  mu_run_test(test_validate_fail_type);
  mu_run_test(test_validate_fail_start);
  mu_run_test(test_validate_fail_end);
  mu_run_test(test_validate_fail_empty);

  mu_run_test(test_parse_ok);
  mu_run_test(test_parse_unknown);
  return 0;
}

int
main(void)
{
  char *result = all_tests();
  if (result != 0) {
    printf("%s\n", result);
  } else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);

  return result != 0;
}
