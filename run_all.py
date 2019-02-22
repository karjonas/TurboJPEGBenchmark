import subprocess

tile_sizes = [64,128]
chunk_sizes = [1,2,4,8,16,32,64,128,256,512,1024]
#num_threads = [8, 16, 32, 64, 128, 256]
#chunk_sizes = [1,2]
num_threads = [4,8, 16]
seconds = 5

results = []

ctr = 0
num_tests = len(tile_sizes)*len(chunk_sizes)*len(num_threads)
for ts in tile_sizes:
    for cs in chunk_sizes:
        for threads in num_threads:
            ctr += 1
            print(str(ctr)+"/"+str(num_tests))

            output = subprocess.run(["./build/benchmark", "--directory", "tiles_"+str(ts), "--width", "15364", "--height", "3424",
             "--seconds", str(seconds), "--omp-chunk-size", str(cs), "--threads", str(threads)], stdout=subprocess.PIPE)
            lines = output.stdout.decode('utf-8').splitlines()
            for line in lines:
                words = line.split()
                if len(words) > 0 and words[0] == "Megapixels/s:":
                    mps = float(words[1])
                    results.append((ts, cs, threads, mps))

def sortMps(elem):
    return -elem[3]

results.sort(key=sortMps)
for result in results:
    print(result)