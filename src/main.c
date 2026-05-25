#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <SEGGER_RTT.h>

#define ROW_COUNT 6
#define COL_COUNT 4

struct key_coord {
	uint8_t phys_row;
	uint8_t phys_col;
	uint8_t logical_row;
	uint8_t logical_col;
};

static const struct gpio_dt_spec rows[] = {
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), row_gpios, 0),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), row_gpios, 1),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), row_gpios, 2),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), row_gpios, 3),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), row_gpios, 4),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), row_gpios, 5),
};

static const struct gpio_dt_spec cols[] = {
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), col_gpios, 0),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), col_gpios, 1),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), col_gpios, 2),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(kbd_matrix), col_gpios, 3),
};

static const struct key_coord key_map[] = {
	{0, 3, 0, 0},
	{1, 0, 1, 0},
	{1, 1, 1, 1},
	{1, 2, 1, 2},
	{1, 3, 1, 3},
	{2, 0, 2, 0},
	{2, 1, 2, 1},
	{2, 2, 2, 2},
	{3, 0, 3, 0},
	{3, 1, 3, 1},
	{3, 2, 3, 2},
	{3, 3, 3, 3},
	{4, 0, 4, 0},
	{4, 1, 4, 1},
	{4, 2, 4, 2},
	{5, 0, 5, 0},
	{5, 1, 5, 1},
	{5, 3, 5, 2},
};

static bool pressed[ROW_COUNT][COL_COUNT];

static const struct key_coord *find_key_coord(int phys_row, int phys_col)
{
	for (size_t i = 0; i < ARRAY_SIZE(key_map); i++) {
		if (key_map[i].phys_row == phys_row &&
		    key_map[i].phys_col == phys_col) {
			return &key_map[i];
		}
	}

	return NULL;
}

static void rtt_print_key(uint8_t row, uint8_t col)
{
	char buf[16];

	snprintk(buf, sizeof(buf), "[%u,%u]\n", row, col);
	SEGGER_RTT_WriteString(0, buf);
}

static int matrix_gpio_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(rows); i++) {
		if (!gpio_is_ready_dt(&rows[i])) {
			return -ENODEV;
		}

		int ret = gpio_pin_configure_dt(&rows[i], GPIO_OUTPUT_INACTIVE);

		if (ret != 0) {
			return ret;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(cols); i++) {
		if (!gpio_is_ready_dt(&cols[i])) {
			return -ENODEV;
		}

		int ret = gpio_pin_configure_dt(&cols[i], GPIO_INPUT | GPIO_PULL_UP);

		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static void matrix_scan_once(void)
{
	for (uint8_t row = 0; row < ROW_COUNT; row++) {
		gpio_pin_set_dt(&rows[row], 1);
		k_busy_wait(50);

		for (uint8_t col = 0; col < COL_COUNT; col++) {
			bool is_down = (gpio_pin_get_dt(&cols[col]) == 1);

			if (is_down && !pressed[row][col]) {
				const struct key_coord *coord = find_key_coord(row, col);

				if (coord != NULL) {
					rtt_print_key(coord->logical_row, coord->logical_col);
				}
			}

			pressed[row][col] = is_down;
		}

		gpio_pin_set_dt(&rows[row], 0);
	}
}

int main(void)
{
	if (matrix_gpio_init() != 0) {
		return 0;
	}

	while (1) {
		matrix_scan_once();
		k_sleep(K_MSEC(5));
	}

	return 0;
}
