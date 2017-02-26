if (redis.call("get",KEYS[1]) == ARGV[1])
then
  return redis.call("set",KEYS[1],ARGV[2])
else
    return nil
end
