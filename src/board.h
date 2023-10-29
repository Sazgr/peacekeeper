#ifndef PEACEKEEPER_BOARD
#define PEACEKEEPER_BOARD

#include "eval.h"
#include "lookup_arrays.h"
#include "magics.h"
#include "move.h"
#include "movelist.h"
#include "options.h"
#include "typedefs.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

enum Piece_types {
    black_pawn,
    white_pawn,
    black_knight,
    white_knight,
    black_bishop,
    white_bishop,
    black_rook,
    white_rook,
    black_queen,
    white_queen,
    black_king,
    white_king,
    empty_square
};

enum Move_types {
    quiet = 1,
    noisy,
    all
};

const int castling_disable[64] {
    14, 15, 15, 15, 12, 15, 15, 13,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    11, 15, 15, 15,  3, 15, 15,  7
};

const u64 zobrist_pieces[13][64] = {{
    0x41c67ea6, 0x10e659d4967eb0e7, 0xd61596462781e494, 0xc544e568c46b9b3d, 0x522d718f94bdf32, 0x81fc45f195fb7483, 0xa5a83ca0d9e2b600, 0x86d5e86e9cfbae39,
    0x714efcd2bf54bc7e, 0x5d3e5b6d0ff6d5df, 0x1ad02d840abd322c, 0x935bf49031dff4f5, 0x86db8101237c228a, 0x5da83d13af1cf0fb, 0x5fedca4e7de14518, 0xb28e9c8cc487eb71,
    0x4d320fb8e201dd56, 0x397c4d38d2bfa1d7, 0x80e9d9b6e2319ac4, 0xf6c8f672e3decdad, 0x175fc0e4e95678e2, 0x70d64e2293728473, 0x8d3bebd9500f9f30, 0xfbc7856c284797a9,
    0xa67c18342c67412e, 0xa9d6c3277566f4cf, 0xb53b0f5c42877e5c, 0x7713826eb3590565, 0xa23495004daa423a, 0x67bdd54ae4880eeb, 0xfec3ef89f73c2448, 0x78e484ccef5992e1,
    0xd909d9c1eeea4806, 0x133313db5656aec7, 0xc95d121de1133cf4, 0xb167c8a7ca7b7c1d, 0xf483852831d2de92, 0x604f04e6ad857063, 0x719c75d499a13460, 0x28f5b34f69d8bd19,
    0x268b70b5b53c51de, 0x3544d8073b54afbf, 0x93ca93477d55368c, 0xb63e1062102f11d5, 0x135f529e1b37adea, 0x55f489d57aae88db, 0xea60ef03de652f78, 0x2c8ea5b8b45bf651,
    0x81e1af43e3dabeb6, 0xdbdb14f06102d7b7, 0x9735e8e07a79cb24, 0xeba033ac8398a68d, 0xe7cc1ae728cc1042, 0x623f40cfe0e33853, 0x38f90129f1a7590, 0x6ad2cab84b761e89,
    0x4b37d292858eee8e, 0x1a29e432fa5f06af, 0x585d782f28d95abc, 0x84352ac87991a45, 0xc7b984dc388f659a, 0x3d28d2b4751f5ecb, 0x43e43fda493f66a8, 0x4311725c8f3615c1,
}, {
    0xd339646648ee4166, 0xba7d0a720431ca7, 0xe87ac12bbf784554, 0xe69334971e4d4cfd, 0x12467fe93d0d0df2, 0x69e30f8c33fadc43, 0xe40d3c7befbe62c0, 0x61cb482436a6bbf9,
    0x916b5ac1adda173e, 0xff4d7e0430e4f99f, 0xd80be3290586eaec, 0xb867c026148e1eb5, 0x69646631c2dc694a, 0x23d959c4592990bb, 0xc5658faa0c6dc9d8, 0x48b1785ab34ef131,
    0x23e99b8c62ffd016, 0x6b9386a09f567d97, 0xa98575b8abe1ab84, 0xc82aa1171d706f6d, 0xad95f310a620d7a2, 0xd1f664dba6fb5c33, 0x3fecd738f18ffbf0, 0x4ccb612d84b19569,
    0x6219f718d358cbee, 0x1934c86c3305888f, 0x4d2e2b915690e71c, 0x87529402ddc51f25, 0xfbc87828f809b8fa, 0x9fa1f4811ddc1eab, 0x34e66e1beb535908, 0xfc018fa87bcd88a1,
    0xaa2e711de3aa6ac6, 0xb029a002b73bfa87, 0x2319e7fc5648fdb4, 0x55c9ed99e7990ddd, 0xe6360bfe14526d52, 0x46572ae223d3b823, 0x182ea0b511524120, 0xc642e086ee9daad9,
    0xb5d54c7ae0060c9e, 0xc309ce751a9fb37f, 0x7269ba1111ea4f4c, 0x2c75c615a5b51b95, 0x845115c8e6c254aa, 0xe7234ea61c06089b, 0x2d964e1dc8131438, 0x2fa1ec87db98dc11,
    0xdd8c2db99491176, 0x686dfc26feb29377, 0xe67c93c420013be4, 0x2de1bd6b371e284d, 0xabf9b0060accf02, 0xefa5c2d56e32f013, 0x6cff26cdf2883250, 0x74546fb2fd31fc49,
    0x18b59e12b29cd94e, 0xdd4df8deb7527a6f, 0x4d48d901046237c, 0xe330908e3a951405, 0x729a0bcb1e713c5a, 0x6d0334a9fe364e8b, 0x496f9fc7d38ffb68, 0x987b1abccd57eb81,
}, {
    0xcba0dc01ef6c426, 0x85dcfee5ba394867, 0xceb91a3de51d6614, 0x6ddab3d18a16bebd, 0xb108fc503cfafcb2, 0xa33ec50713880403, 0xd130b02e9f74cf80, 0x1f5357778f589b9,
    0x3bd01099ce9831fe, 0xc1105cd77e7cdd5f, 0xb60ab9133d1763ac, 0xce58da33e65c0875, 0xdff2419d5f41700a, 0xdf0b2d16b0bbf07b, 0x1eb85edbd6d0e98, 0x870fa8fec371b6f1,
    0xabd39d6c8c8e82d6, 0xf4b5ff7ccc0f1957, 0xefab776c2c707c44, 0x1f57bd269259d12d, 0x2afb4997e3c7f662, 0x59ba8f4a5b01f3f3, 0xc0b58cd6b91b18b0, 0xd149700cda2f5329,
    0x69bf64ab0c3316ae, 0xa18e62d17b3ddc4f, 0xca0bed86c6910fdc, 0x981b9c1bdec0f8e5, 0xfe6f3e2f4a1defba, 0x54b4cfbd51a5ee6b, 0xe6706a5be40d4dc8, 0xf63f0ba6180d3e61,
    0x3cf6882126ab4d86, 0xc6e77712a4330647, 0xaf15506578d7e74, 0x8fcbc6eba57e5f9d, 0xffa694c6c85ebc12, 0x25e0f0b6358fbfe3, 0x1aa4eecda73e0de0, 0x83827209b8e65899,
    0x68c103214868875e, 0x484304733e74773f, 0xe3e1e2744da6280c, 0x87d819dbb53ae555, 0x77d6138c10b1bb6a, 0xeead09c620c3485b, 0x363673c08493b8f8, 0x1503fd957d1181d1,
    0x4056b2420ea82436, 0x740a0dd530640f37, 0x4cabadc4d2c76ca4, 0x53ca9dda2cdb6a0d, 0xe580173146ca4dc2, 0x31b101c32de067d3, 0x282ae01c860af10, 0xebea290a3ce19a09,
    0x2294f68214f3840e, 0xcc76a0a4cebfae2f, 0x4b6b30a51609ac3c, 0x4fbd8337c700cdc5, 0x5856fc402567d31a, 0x81013d9c6fa2fe4b, 0xada3a040eae35028, 0x3758665f6c258141,
}, {
    0xe817d467f2a006e6, 0xc1dd65b4cc213427, 0x91ebb419453146d4, 0xa85868f01587f07d, 0xbcc5fffc53d5ab72, 0xc2bef7c55862ebc3, 0xd8e11a53a1c5fc40, 0x86a015138da81779,
    0x5cac2c74684f0cbe, 0xdb5f24b8987e811f, 0xaa30f1b0362e9c6c, 0xc86564dad09b235, 0xbc81b58aeb6b36ca, 0x77c2edb69194103b, 0x989807baa19f1358, 0xa8788ca796b03cb1,
    0xcd103efcbd6df596, 0x26af41bb30a97517, 0x3cc8af52c09e0d04, 0xa3c6f878405af2ed, 0x10c54cd52d0bd522, 0xcebb82c243464bb3, 0x9d02becd0f70f570, 0x53d357b34280d0e9,
    0x6e3d1a434db6216e, 0xf4ce09b05dcff00f, 0x48e83774c747f89c, 0x56b0993aac0c92a5, 0x20f1615c66a6e67a, 0x55f86139cba57e2b, 0x27c988b0a22a0288, 0xea3f1bc055d8b421,
    0x43e1d66446acf046, 0xfdfb68fe64fbd207, 0x8a6f422df1a0bf34, 0xb16be00ef1eb715d, 0x802ed7f508b7cad2, 0x187a04a2e67987a3, 0x8be0b90474249aa0, 0x41fd41dad272c659,
    0xc1939f6c9523c21e, 0x4647649e2692faff, 0xa72b2e801548c0cc, 0xd205c23e24806f15, 0x30f77b1eebc5e22a, 0x54aa717a44a6481b, 0xcb24e2e484a71db8, 0x3e4343841a85e791,
    0xc41e82b102b7f6f6, 0xd368642fadd74af7, 0x69fd4dd14f8c5d64, 0xa3a612a242906bcd, 0xe3c98670c5e48c82, 0x13cb0c193ab9f93, 0xe0e907abe963ebd0, 0x58c787770444f7c9,
    0x965dc13b8352eece, 0x272b09213066a1ef, 0x47107e5fcee3f4fc, 0x5a28e558029c4785, 0x94062a4cd03329da, 0x1fc96f56f5256e0b, 0x1a31029baff964e8, 0xa1c1f96add5ed701,
}, {
    0x4720de4b2aa09a6, 0x750af4c67dbadfe7, 0xe3b064354c73e794, 0xba45776e8e60e23d, 0xeba041d49c5d1a32, 0xd6f62d3e64b9383, 0xe5e6f34c6f71e900, 0xd6e0ffc35e7e6539,
    0x77d39a6f81bea77e, 0x40567cfedea9e4df, 0x22923ae0358c952c, 0xbd357e632e571bf5, 0xcbc2fdd21a19bd8a, 0xd47ccf719771effb, 0xc9a7ff9089c3d818, 0x692b21db8eca8271,
    0xdac02bbd145e2856, 0x878fe4c764e590d7, 0x22cf7cf8852a5dc4, 0xcc72aef6e533d4ad, 0x14b280bccac73e2, 0x6b4bbc30b3886373, 0xadd6742d1d519230, 0xab0b995997660ea9,
    0xb76599eecea1ec2e, 0xbc543bd7aa7bc3cf, 0xe53079364d75a15c, 0xe43ceeabfb67ec65, 0x283db3a30649d3a, 0x943b0c42979acdeb, 0x7ee0128fa6697748, 0x1f6b28b986efe9e1,
    0xa1ca260c926f5306, 0x19918e2701565dc7, 0x15cb5f9ff142bff4, 0xd0b0fef97aa0431d, 0x56241c94501d9992, 0x7122d671fa510f63, 0x95e2386650c5e760, 0x5825e8fd0502f419,
    0x8718eb852cf7bcde, 0x16ca89f812bb3ebf, 0x1ce65c160d92198c, 0x50a485029945b8d5, 0xff80aca98abec8ea, 0x95c0ed09036f07db, 0x2a3584c0f90d4278, 0xf3b9502df5b60d51,
    0xeb44689ef43889b6, 0x4af438c8eecc46b7, 0x4353f64313100e24, 0xbf801c6015fd2d8d, 0xea17a5fe88bb8b42, 0xe43a9a72d3549753, 0xa47aad20de51e890, 0xa8da7a220d1c1589,
    0x91e6a5ae947b198e, 0xde69d5458c0755af, 0x8e6329b48f94fdbc, 0xf109a74983278145, 0x9f3d18a36193409a, 0xf4a920967a7d9dcb, 0x1d6d5b9d039239a8, 0x92d2bcad52c3ecc1,
}, {
    0xa948e7aa0dd4cc66, 0xa5aa2e1ab6c64ba7, 0xd4937ba427a54854, 0xc17f4a2c826193fd, 0xd753c2b8f15148f2, 0xc44e8aa96101fb43, 0xc4532e01413895c0, 0xd5f6c831953872f9,
    0xb84f176de1a7023e, 0x9818634770bf089f, 0x1d83beb63ff14dec, 0x7c65747ff00445b5, 0x77964e9b5e0d044a, 0xf5fedaf71e158fbb, 0x7dff448e069b5cd8, 0x821a0538cd808831,
    0xc8c81bca701f1b16, 0x5ab6214ac0836c97, 0xd86337b456d56e84, 0x3cbf0460fea4766d, 0x14572027cd69d2a2, 0x2ed58fdcbf883b33, 0x54213dbecb7ceef0, 0x26338788729f0c69,
    0x6fdf512f85b676ee, 0x8886cb4bf101578f, 0xb84371090dda0a1c, 0xbaacf64842930625, 0x379b3534a1713fa, 0x67afe4f58145ddab, 0x295c033d318bac08, 0xb012cedbbd12dfa1,
    0x7bff0e8c18b275c6, 0x54a84b1b4102a987, 0x55203d83e33380b4, 0x222a2de4ad5cd4dd, 0xf4d3df08d9502852, 0x8c301736f4d65723, 0x40386ae6d5e1f420, 0xd36fa532da56e1d9,
    0xb9a3c3b836a4779e, 0x16b67ba902ad427f, 0xd3c635aa9b42324c, 0x631eb8ea794ac295, 0x1e91f804c05c6faa, 0x863c4c7a98dd879b, 0xce9c658bd2862738, 0xaa20ba651061f311,
    0x3c0db27f21e9dc76, 0x142858512b030277, 0xcac95d9e5a127ee4, 0x4f9b786404e1af4d, 0xc679d022fa0f4a02, 0x7bd390b2e09b4f13, 0x848c43cbefeaa550, 0x64072463d126f349,
    0x416f901f9f2c044e, 0x5a4949195161c96f, 0xbcb89c5a6cdcc67c, 0x7dd260d29e627b05, 0x930e756dc48175a, 0xcb6ba4c9ab6b8d8b, 0x118b8038866dce68, 0x7f50ab5cbe14c281,
}, {
    0xdf8d81d872e04f26, 0xaf6409bf1f037767, 0x39423fd2c3856914, 0x714f7d2d3f4a05bd, 0xc50cd441ed7237b2, 0xb76a1ce62c462303, 0x38407ca10fda0280, 0xe306c0a89b9640b9,
    0x8a7ce3d70ec81cfe, 0xd7aec7352e7dec5f, 0xaa0cf4381a1cc6ac, 0x7bf2948b37d12f75, 0xec64f31fea050b0a, 0x8e298598413eef7b, 0x2ab5b73068e5a198, 0x87432f3534924df1,
    0x157d1e9e6f70cdd6, 0x2e7024ec5b430857, 0x82fa5767d25f3f44, 0x1efbe08ca6cd82d, 0x152cca7dfa03f162, 0x43aef9273b05d2f3, 0xcbbe2ac0c2b30bb0, 0xb384fbd82debca29,
    0xa1377bbc29b3c1ae, 0x671e9078120ab4f, 0x9717ac4d7d3532dc, 0x7c4d5233b74ddfe5, 0x31824df3167e4aba, 0xcaf4b8021466ad6b, 0xd30cd48c4450a0c8, 0xc70e8bb8ca019561,
    0x2bfa6f61a8365886, 0xf8cf1457abc0b547, 0x6c924bc14330174, 0x576e850db7e1269d, 0x4ae537cf9f0f7712, 0x9c91f90f19c95ee3, 0x9b43b8205c38c0e0, 0x5e60febe9c2e8f99,
    0x29ff293798e9f25e, 0x9f0728beb629063f, 0xd437c304e3190b0c, 0x6c9a8772ea4f8c55, 0x43701b7a1f5ed66a, 0x2186405900b1c75b, 0x8bc83139c1d1cbf8, 0x401ebbb32c4998d1,
    0x551973818a8bef36, 0x376656565a3b7e37, 0x7893f7522153afa4, 0x90575a722cfdf10d, 0x47b92ef449fc8c2, 0xf4de1c576f3fc6d3, 0xe13f1ef26ee2210, 0xea0f73848a259109,
    0x5d5cf467ba25af0e, 0x71893579b035fd2f, 0x6792d78b3b7b4f3c, 0xe6fc82e56a0d34c5, 0xc6c3029b0311ae1a, 0x6b7f33d0f3af3d4b, 0xa86eb250994c2328, 0xd2b960d6d1115841,
}, {
    0xa5aac9fd108c91e6, 0xe31998f1e326327, 0x43442348ccd449d4, 0x2d138757d2da377d, 0x1f03df80eb7fe672, 0xfc4110146bd80ac3, 0xd9c7556194162f40, 0x45f540d29b57ce79,
    0x1f8236574fe1f7be, 0x49cfe630b7a6901f, 0x2df7301e48ceff6c, 0xcad196080b7dd935, 0x4c172c69b0c1d1ca, 0xa5060907dcae0f3b, 0x156f1e13c162a658, 0x42db71de65bfd3b1,
    0xddf9a5cf71134096, 0x7df826110ce46417, 0x3b75d3f5487d004, 0xa6eddc94464cf9ed, 0xcd04038bdd3ad022, 0xc5232a4ab9c12ab3, 0x55d17da86bb3e870, 0x5217ccb0e30c47e9,
    0x6ef5c6403159cc6e, 0xc33af0ba6a99bf0f, 0x9ab0c846d0471b9c, 0x9bf1fe9e4f5879a5, 0x42347ae5b85a417a, 0x8f45d2699cbd3d2b, 0xd5520b9e9f785588, 0x16a62bea3f7c0b21,
    0xb76daad8cfbafb46, 0xb42ef20689508107, 0xdfa6afa591014234, 0xe75cb47587ed385d, 0x2bdf023e5c1b85d2, 0x32412ee16cea26a3, 0xc10b5613fc8a4da0, 0x4ca22e225449fd59,
    0x711014d9fa882d1e, 0x8367a3ecacee89ff, 0x680939fec9d6a3cc, 0xa7e8ee94d2141615, 0xc7508283fa85fd2a, 0xd9bb916ff6abc71b, 0xc567703e37b030b8, 0x2d808e19cb2cfe91,
    0xff91ee52ecdec1f6, 0xaa0781043435b9f7, 0x89033c782593a064, 0x9944e6826c11f2cd, 0xa6e206fd532d0782, 0xa66aeceff301fe93, 0x9e0173634c1c5ed0, 0x2ce6747c31d7eec9,
    0xf653d0b4bc2819ce, 0xbad6a3d99843f0ef, 0xed4514c3903097fc, 0xc99b495fbbe7ae85, 0x363a918458b004da, 0x162791be7f08ad0b, 0x94d88d765ced37e8, 0xd2726660fd79ae01,
}, {
    0x454715d1d59994a6, 0x4f716cf3dc130ee7, 0x91fabe68b051ea94, 0x678424370ad2293d, 0x529debc0063a5532, 0x632b83680377b283, 0x86e4b73f46ad1c00, 0xeb19eb797e3d1c39,
    0xf8cd6e9fabb4927e, 0xa14467286bf8f3df, 0x8e3bf59410c7f82c, 0x5d226dd530ca42f5, 0x3966d0136503588a, 0x9fd27b1a8c22eefb, 0x223dc2b2e0d26b18, 0x670db699c2c91971,
    0x53db4fd093c67356, 0x378a379b6d277fd7, 0x3bb87c71fa0f20c4, 0xd41cf5bd3004dbad, 0x3cb8a5b7c1ce6ee2, 0xf171dc1b8f7a4273, 0xbb4a20e1ef3f8530, 0x2113030a6bc085a9,
    0xdf70b11bd368972e, 0x8524f48a7d2c92cf, 0x402372dafbcfc45c, 0x8b652383c072d365, 0x16a64438126af83a, 0x4d9f893f26098ceb, 0xd83e85a4c3c2ca48, 0x527af8d16f4240e1,
    0x3567a0c9de005e06, 0xed9cc3bfe1720cc7, 0xe0a22d8265e42f4, 0xe5da4da0cb410a1d, 0x3f92c3438b345492, 0xf591ac1eb1f8ae63, 0xdc656de98f969a60, 0xec741dfcc692b19,
    0xc6e809dac23f27de, 0x8e511f4c26bdcdbf, 0xdacfaf44f43afc8c, 0x832c8284d6585fd5, 0x16e2448d6491e3ea, 0x9b1f188cf68b86db, 0x4d58844b64e15578, 0x5dec89d22ecc2451,
    0x626bfdcc7a254b6, 0xe06794e430b1b5b7, 0x3452b494e3925124, 0xf32c6a805fddb48d, 0x9fde62fd0770642, 0xca2708859fa1f653, 0x7c18ee57e8355b90, 0x192f69b281fe0c89,
    0x3e7670493bf3448e, 0x1f814201b94ba4af, 0x49872682bfbca0bc, 0x78166ccb29b1e845, 0xb16aa4c81fe31b9a, 0x4b7e76963937dccb, 0x89b1b3a9b2110ca8, 0xd0dd40e8750dc3c1,
}, {
    0x917e7dfd70c75766, 0x48e6b4a440657aa7, 0xc0a8882f9abe4b54, 0xcc5c7db974f1dafd, 0x85d1e542186183f2, 0x7885e7e96e51a43, 0xfc13aebd605ec8c0, 0x8f8fb446ee0629f9,
    0x5a690825e8ffed3e, 0x6caf36506b35179f, 0x169206f776c7b0ec, 0x52daefed2d766cb5, 0xc64ea70879899f4a, 0xbdcba586ab5d8ebb, 0x13cfce2757f4efd8, 0x300dfbe46d6e1f31,
    0x8b2972b1b64a6616, 0xb653dcabd3cc5b97, 0x7c9e03019fb53184, 0xf1004e1105547d6d, 0x686f0010b27ecda2, 0x93845dc7cff11a33, 0x6845db903615e1f0, 0x1de9d02c61c88369,
    0x4122b32406a021ee, 0x1e708d34899268f, 0xace82b52b48f2d1c, 0xdddda66b805ced25, 0x2d17cf7c7706efa, 0x988d81677c0b9cab, 0x6e36849df1efff08, 0x3635a6576b1436a1,
    0x47536959e1c680c6, 0x167544497be55887, 0xc40785e6610a03b4, 0xd0d2cd57ef9c9bdd, 0x4a2c50266719e352, 0xd32b24816cb4f623, 0x273200afae1da720, 0xf9604388e4c18d9,
    0xf808689a16cee29e, 0x326ba81c2356d17f, 0x6fcb8716c706154c, 0xe0f7ef735cdc6995, 0xd253e3b230428aaa, 0x772a763f3c11069b, 0x304465123a253a38, 0x30c7030d58e70a11,
    0x278fca055996a776, 0x43a9309d876f7177, 0xf513be57980fc1e4, 0xd2c2580b6621364d, 0xf1526272273dc502, 0xfa53bb5b68dfae13, 0x3710881343f91850, 0x9ea8a0bef457ea49,
    0xbe81ee3c90472f4e, 0x57628dd0830d186f, 0xdcb7990adedf697c, 0xbcc941d092be205, 0x4905257c5b6af25a, 0x3a01b60dcdfccc8b, 0x882d3a193977a168, 0x1a0f3ec2298d9981,
}, {
    0x9455f8d350d5da26, 0x75622664f2e9a667, 0x363e4ef578d96c14, 0x7a2115f15ef94cbd, 0x5692a402bcb572b2, 0x1670b11f89e04203, 0xe4599aa5d9eb3580, 0x3fe0a5845472f7b9,
    0x42df4ce48e8407fe, 0x37720e2951afb5f, 0xbcd936993f8e29ac, 0xbac9312647425675, 0x90568c452114a60a, 0xde749da4561dee7b, 0xe6a765e9778a3498, 0x50401313476ee4f1,
    0x572466df775f18d6, 0x7ffd5c605892f757, 0x9a07827ae23a0244, 0xfd6fb2b103fbdf2d, 0x17ed0d0e7a0bec62, 0x301864974ee5b1f3, 0x2118464ce8f6feb0, 0x6364436e1ee44129,
    0xfb401c6281c06cae, 0x7a144fe61c9f7a4f, 0x643cc3196f4555dc, 0x8c3d7c28c4d6c6e5, 0xd3850ff33a2aa5ba, 0xb267b7592a836c6b, 0x2c87c182abff3c8, 0x62b800ad04b1ec61,
    0x8c8fcfbaa9cd6386, 0xba02cd56e06a6447, 0xedee7c1d8dc48474, 0x7836656722bfed9d, 0x5fa63546ea8c3212, 0x94674bcde0defde3, 0x90688c1bb0df73e0, 0xbc87a0ede3b2c699,
    0x7796815cdef75d5e, 0x68f0a2816279953f, 0x8a0ba60666f7ee0c, 0x5ca43b4c8b603355, 0xc895cb7ef057f16a, 0xaafc8a97c2fc465b, 0x79a41d02683bdef8, 0xd80ae7b40b3dafd1,
    0x3b58de6fa17bba36, 0xbaf408b5302eed37, 0xb1ee185a3fcbf2a4, 0xdcbfe369c9c780d, 0x8350bc78824143c2, 0xe11447ae027b25d3, 0xa2b0b9b268279510, 0xa420828c2a58809,
    0x258c783acfe3da0e, 0xb5b6c0fa25484c2f, 0xd28c9320c258f23c, 0x975d867670159bc5, 0x3da83d7ce07891a, 0xfc6f274ca9177c4b, 0x3586d5e253e0f628, 0xc3ed056accb92f41,
}, {
    0xf2273513a4851ce6, 0x16d4a0c85b5f9227, 0xbbaf712cf7634cd4, 0x51c670d4d6a87e7d, 0x96fdb2764df62172, 0xa7b0d2fc002929c3, 0x718530c96c126240, 0xcacd65dadb438579,
    0x476ec81ae300e2be, 0xc39230de896a9f1f, 0x1d24f77cefdb626c, 0xcfada8f283ee0035, 0x957c4b964e646cca, 0x251c1c2d68240e3b, 0xb01d889050523958, 0x40321e12f28b6ab1,
    0xacbd91e335c48b96, 0xd58cd594d33b5317, 0xd0c7b8b51e5d9304, 0x981c131229bb00ed, 0xae686951a335cb22, 0x42209baba01809b3, 0x11b47fe870a2db70, 0x7b33bff6bcd3bee9,
    0x584db8d5bb89776e, 0x8155c5ca08ff8e0f, 0x7a549e7d60b23e9c, 0x48d7ba9a83a060a5, 0x4da30e798d599c7a, 0xcbbd46dc7d30fc2b, 0x35809df02ef2a888, 0x1b925a0acddb6221,
    0xb75f8ceac4d50646, 0xca2cdb4956c13007, 0xf9a9dd45b94dc534, 0xe6b8865e526aff5d, 0x5562dcddd04b40d2, 0x9fd5d5921236c5a3, 0x7ab86f49b09c00a0, 0x3b460b00d65d3459,
    0x4d40670cc178981e, 0x74cf054663e618ff, 0xc38f61b8b8d086cc, 0x6f9ec77847a3bd15, 0xb8998fb0f792182a, 0x74e5ace9470d461b, 0xb7f1060a5fe543b8, 0xfd61d26c7901591,
    0x5fb6293b5e118cf6, 0x37ecdb4de2b028f7, 0x5f7729e19786e364, 0xa0740348e10f79cd, 0xb5e89a7fcc418282, 0x44d15573e0345d93, 0x98b764e91d80d1d0, 0x239f8526e6a6e5c9,
    0xa2245144d18944ce, 0x6ea77cc88fbd3fef, 0x94f376cbfee93afc, 0x475c1ce4342f1585, 0xbcc625f1fa78dfda, 0x89ae4cabf647ec0b, 0xb11bc8d1220d0ae8, 0x4b69df46d0508501,
}, {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
}};

const u64 zobrist_black = 0x9c534ab5a951fa6;

const u64 zobrist_enpassant[65] = {
    0x7d8b8eeea1873de7, 0x1d8cd623831bed94, 0x21f77dfda9bf703d, 0xb90ed449e6e39032, 0x44c7436edd7fd183, 0x4d6042bf8f944f00, 0x89f07e136c37d339, 0x734f790ced367d7e,
    0x37f2c8c9a7e402df, 0x4b25ae18cc6f5b2c, 0x6d2d131fa93969f5, 0xc794059b438f38a, 0xcd5f94fd7d2fedfb, 0x29efaa91b30cfe18, 0xcc2acd27d083b071, 0xc1783123103abe56,
    0xe76312a2db856ed7, 0x788f389270dfe3c4, 0xe823dabd3451e2ad, 0xfc890a6378bc69e2, 0x921af3c017482173, 0xb839a229f5d97830, 0x13d3327d1556fca9, 0x99783330eabb422e,
    0xfb35cbfbdd7961cf, 0x1217b56f7d95e75c, 0x907e6a6b7279ba65, 0xa16b7f1aa3bd533a, 0x8dacf2cb7fd44beb, 0x797727117f481d48, 0xae7cb871185097e1, 0xa9f24275819d6906,
    0x693150efe6a9bbc7, 0x53c60e61b065c5f4, 0xf10e29512c5dd11d, 0x84aa75bc93170f92, 0x5da634e5c47c4d63, 0xc47c0f7c86134d60, 0xde52272300b6219, 0x5d9281f8251292de,
    0x251559b675c5cbf, 0x702731a5614fdf8c, 0x156f129a376706d5, 0x834deaf658b0feea, 0xecd1a388440405db, 0xc9bfc65751e16878, 0xe8699dfdcf9e3b51, 0xc7215bc50e181fb6,
    0xf62723e816b324b7, 0x6ee00b9d1c009424, 0x588e9e7cd13a3b8d, 0x6ead1f0daffe8142, 0x5d72221d35cb5553, 0x5d3da8c1ecc4ce90, 0x3fc709601a1c0389, 0x738f94702bf76f8e,
    0x3d4835db722bf3af, 0x77df4216e95043bc, 0xff27f41eeb384f45, 0x7a079683237ef69a, 0xfa87bb76a14e1bcb, 0x239e8e2084bbdfa8, 0xa8aac96266139ac1, 0x9819a77421c5e266,
    0,
};

const u64 zobrist_castling[65] = {
    0x94570c45ad20a9a7, 0xdda6a6c148c34e54, 0x33bbc26965fe21fd, 0x5c984d23623dbef2, 0x79a9493cc5a43943, 0x749442a67d30fbc0, 0xa1fd5ed6b10fe0f9, 0x8c8c5c373e4d83e,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0x7f567f6f1047269f, 0x55b0c115da0a13ec, 0x3a5a11973ce493b5, 0x4b24b43dc5523a4a, 0x446c0ed1f1018dbb, 0xd3181302307a82d8, 0x790b5cae0317b631, 0xdd72e4a1e581b116,
    0,
};

class NNUE;

class Position {
private: //simple templates
    template <bool side> inline u64 pawns_forward_one(u64 pawns);
    template <bool side> inline u64 pawns_backward_one(u64 pawns);
    template <bool side> inline u64 pawns_forward_two(u64 pawns);
    template <bool side> inline u64 pawns_backward_two(u64 pawns);
    template <bool side> inline u64 pawns_forward_left(u64 pawns);
    template <bool side> inline u64 pawns_backward_left(u64 pawns);
    template <bool side> inline u64 pawns_forward_right(u64 pawns);
    template <bool side> inline u64 pawns_backward_right(u64 pawns);
    template <bool side> inline u64 promotion_rank();
    template <bool side> inline u64 second_rank();
public:
    template <bool side_to_attack> u64 attack_map(u64 occupied);
    bool check();
    template <Move_types types, bool side> void gen_legal(Movelist& movelist);
    void legal_moves(Movelist& movelist);
    void legal_noisy(Movelist& movelist);
    void legal_quiet(Movelist& movelist);
    void print_bitboard(u64 bits);
    void print(std::ostream& out);
    template <bool update_nnue = false> void make_move(Move move, NNUE* nnue = nullptr);
    template <bool update_nnue = false> void undo_move(Move move, NNUE* nnue = nullptr);
    inline void make_null();
    inline void undo_null();
    template <bool update_nnue, bool update_hash> inline void fill_sq(int sq, int piece, NNUE* nnue = nullptr);
    void load(std::vector<int> position, bool stm = true);
    std::string export_fen();
    bool load_fen(std::string fen_pos, std::string fen_stm, std::string fen_castling, std::string fen_ep, std::string fen_hmove_clock = "0", std::string fen_fmove_counter = "1");
    bool parse_move(Move& out, std::string move);
    int static_eval();
    int static_eval(NNUE& nnue);
    u64 rand64();
    void zobrist_update();
//private:
    u64 positive_ray_attacks(u64 occupied, int direction, int square);
    u64 negative_ray_attacks(u64 occupied, int direction, int square);
    u64 general_ray_attacks(u64 occupied, int direction, int square);
    u64 diagonal_attacks(u64 occupied, int square);
    u64 antidiagonal_attacks(u64 occupied, int square);
    u64 rank_attacks(u64 occupied, int square);
    u64 file_attacks(u64 occupied, int square);
    u64 classical_rook_attacks(u64 occupied, int square);
    u64 classical_bishop_attacks(u64 occupied, int square);
    void slider_attacks_init();
    u64 rook_attacks(u64 occupied, int square);
    u64 bishop_attacks(u64 occupied, int square);
    u64 queen_attacks(u64 occupied, int square);
    bool square_attacked(u64 occupied, int square, bool side);
    u64 attacks_to_square(u64 occ, int square);
    bool check(u64 occupied);
    bool draw(int num_reps);
    u64 checkers(u64 occupied);
    u64 xray_rook_attacks(u64 occupied, u64 blockers, int from_square);
    u64 xray_bishop_attacks(u64 occupied, u64 blockers, int from_square);
    u64 hashkey() {return hash[ply];}
public:
    u64 pieces[13] {
        0x000000000000ff00,
        0x00ff000000000000,
        0x0000000000000042,
        0x4200000000000000,
        0x0000000000000024,
        0x2400000000000000,
        0x0000000000000081,
        0x8100000000000000,
        0x0000000000000008,
        0x0800000000000000,
        0x0000000000000010,
        0x1000000000000000,
        0xffff00000000ffff,
    };
    const u64& occupied{pieces[12]};
    int board[64] {
        6,  2,  4,  8, 10,  4,  2,  6,
        0,  0,  0,  0,  0,  0,  0,  0,
        12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,
        1,  1,  1,  1,  1,  1,  1,  1,
        7,  3,  5,  9, 11,  5,  3,  7
    };
    int king_square[2] {4, 60};
    bool side_to_move{true};
    int ply{};
    int enpassant_square[1024] {64};
    int castling_rights[1024][4] {0, 7, 56, 63};
    int halfmove_clock[1024] {0};
    u64 hash[1024] {};
    int eval_phase();
    int mg_static_eval{};
    int eg_static_eval{};
    Position();
    Position& operator=(Position& rhs) {
        memcpy(pieces, rhs.pieces, sizeof(u64) * 13);
        const_cast<u64&>(occupied) = pieces[12];
        memcpy(board, rhs.board, sizeof(int) * 64);
        for (int i{}; i < 2; ++i) {
            king_square[i] = rhs.king_square[i];
        }
        side_to_move = rhs.side_to_move;
        ply = rhs.ply;
        enpassant_square[ply] = rhs.enpassant_square[ply];
        memcpy(castling_rights[ply], rhs.castling_rights[ply], sizeof(int) * 4);
        halfmove_clock[ply] = rhs.halfmove_clock[ply];
        hash[ply] = rhs.hash[ply];
        mg_static_eval = rhs.mg_static_eval;
        eg_static_eval = rhs.eg_static_eval;
        return *this;
    }
};

template <bool side> inline u64 Position::pawns_forward_one(u64 pawns) {
    if constexpr (side) return pawns >> 8;
    else return pawns << 8;
}

template <bool side> inline u64 Position::pawns_backward_one(u64 pawns) {
    if constexpr (side) return pawns << 8;
    else return pawns >> 8;
}

template <bool side> inline u64 Position::pawns_forward_two(u64 pawns) {
    if constexpr (side) return pawns >> 16;
    else return pawns << 16;
}

template <bool side> inline u64 Position::pawns_backward_two(u64 pawns) {
    if constexpr (side) return pawns << 16;
    else return pawns >> 16;
}

template <bool side> inline u64 Position::pawns_forward_left(u64 pawns) {
    if constexpr (side) return (pawns & ~0x0101010101010101) >> 9;
    else return (pawns & ~0x0101010101010101) << 7;
}

template <bool side> inline u64 Position::pawns_backward_left(u64 pawns) {
    if constexpr (side) return (pawns & ~0x0101010101010101) << 7;
    else return (pawns & ~0x0101010101010101) >> 9;
}

template <bool side> inline u64 Position::pawns_forward_right(u64 pawns) {
    if constexpr (side) return (pawns & ~0x8080808080808080) >> 7;
    else return (pawns & ~0x8080808080808080) << 9;
}

template <bool side> inline u64 Position::pawns_backward_right(u64 pawns) {
    if constexpr (side) return (pawns & ~0x8080808080808080) << 9;
    else return (pawns & ~0x8080808080808080) >> 7;
}

template <bool side> inline u64 Position::promotion_rank() {
    if constexpr (side) return 0x000000000000FF00;
    else return 0x00FF000000000000;
}

template <bool side> inline u64 Position::second_rank() {
    if constexpr (side) return 0x00FF000000000000;
    else return 0x000000000000FF00;
}

inline void Position::make_null() {
    ++ply;
    hash[ply] = hash[ply - 1];
    hash[ply] ^= zobrist_enpassant[enpassant_square[ply - 1]];
    hash[ply] ^= zobrist_black;
    side_to_move = !side_to_move;
    memcpy(castling_rights[ply], castling_rights[ply - 1], sizeof(int) * 4);
    enpassant_square[ply] = 64;
    hash[ply] ^= zobrist_enpassant[64];
}

inline void Position::undo_null() {
    side_to_move = !side_to_move;
    --ply;
}

std::ostream& operator<<(std::ostream& out, Position& position);

#endif
