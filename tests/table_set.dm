t = {"abc": 1, "def": 2, "ghi": 3, "jkl": 4, "mno": 5}
for i = 0, i < 20000000, i = i + 1 do
	t["abc"] = i
end

nil
