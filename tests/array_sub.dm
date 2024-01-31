a = [0] * 100000
for i = 0, i < 100000, i = i + 1 do
	a[i] = i % 1000
end

for i = 0, i < 1000, i = i + 1 do
	a - [i]
end

nil
