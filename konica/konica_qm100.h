int konica_qm100_initialize(void);
int konica_qm100_number_of_pictures(void);
int konica_qm100_take_picture(void);
int konica_qm100_delete_picture (int picNum);
struct Image *konica_qm100_get_picture (int picNum, int thumbnail);
struct Image *konica_qm100_get_preview (void);
int konica_qm100_configure (void);
char *konica_qm100_summary(void);
char *konica_qm100_description(void);
