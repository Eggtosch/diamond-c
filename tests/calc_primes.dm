primes = []

function is_prime(n)
	if n < 3 or n % 2 == 0 then
		return false
	end

	for i = 3, i * i < n, i = i + 2 do
		if n % i == 0 then
			return false
		end
	end

	true
end

for i = 0, i < 200000, i = i + 1 do
	if is_prime(i) then
		primes = primes + [i]
	end
end

nil
