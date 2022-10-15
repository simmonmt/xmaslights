package main

import (
	"reflect"
	"testing"
)

func TestParsePixelIDs(t *testing.T) {
	type TestCase struct {
		in   string
		want []int
	}

	testCases := []TestCase{
		TestCase{
			in:   "2..4",
			want: []int{2, 3, 4},
		},
		TestCase{
			in:   "3",
			want: []int{3},
		},
		TestCase{
			in:   "3,9",
			want: []int{3, 9},
		},
		TestCase{
			in:   "all",
			want: []int{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
		},
	}

	invalids := []string{
		"invalid",
		"3,",
		"4,invalid",
		"-1..4",
		"4..1",
		"8..15",
		"15..20",
	}

	for _, invalid := range invalids {
		testCases = append(testCases, TestCase{in: invalid, want: nil})
	}

	for _, tc := range testCases {
		t.Run(tc.in, func(t *testing.T) {
			got, err := parsePixelIDs(tc.in, 10)
			if tc.want == nil {
				if err == nil {
					t.Errorf(`parsePixelIDs("%s") = %v, %v, want _, non-nil`,
						tc.in, got, err)
				}
			} else {
				if err != nil || !reflect.DeepEqual(got, tc.want) {
					t.Errorf(`parsePixelIDs("%s") = %v, %v, want %v, nil`,
						tc.in, got, err, tc.want)
				}
			}
		})
	}
}
