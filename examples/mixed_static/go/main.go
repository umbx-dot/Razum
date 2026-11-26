package main

import "C"

//export go_step
func go_step(value C.int) C.int {
	return value + 4
}

func main() {}
