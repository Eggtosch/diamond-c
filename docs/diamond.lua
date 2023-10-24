vim.lsp.start({
	cmd = { "/home/oskar/programs/diamond-c/lsp.rb", "--lsp" },
	root_dir = vim.fn.getcwd(),
})
