module mod(
	input in,
	output out
);

assign out = in | 0;

endmodule


module basic_module_inst(
	input  in,
	output out
);

	mod mod_1(
		.in(in),
		.out(out)
	);

endmodule