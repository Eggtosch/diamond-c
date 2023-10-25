vim.lsp.start({
	cmd = { "/home/oskar/programs/diamond-c/bin/diamond", "--lsp" },
	root_dir = vim.fn.getcwd(),
})
