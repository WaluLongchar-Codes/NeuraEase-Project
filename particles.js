// particles.js
class Particles {
    constructor(container, options = {}) {
        this.container = container;
        this.options = {
            particleCount: options.particleCount || 200,
            particleSpread: options.particleSpread || 10,
            speed: options.speed || 0.1,
            particleColors: options.particleColors || ['#ffffff', '#ffffff'],
            particleBaseSize: options.particleBaseSize || 100,
            moveParticlesOnHover: options.moveParticlesOnHover || true,
            alphaParticles: options.alphaParticles || false,
            disableRotation: options.disableRotation || false,
            ...options
        };
        
        this.init();
    }

    hexToRgb(hex) {
        hex = hex.replace(/^#/, '');
        if (hex.length === 3) {
            hex = hex.split('').map(c => c + c).join('');
        }
        const int = parseInt(hex, 16);
        const r = ((int >> 16) & 255) / 255;
        const g = ((int >> 8) & 255) / 255;
        const b = (int & 255) / 255;
        return [r, g, b];
    }

    async init() {
        // Load OGL from CDN
        await this.loadOGL();
        this.createParticles();
    }

    loadOGL() {
        return new Promise((resolve, reject) => {
            if (window.OGL) {
                resolve();
                return;
            }
            
            const script = document.createElement('script');
            script.src = 'https://cdn.jsdelivr.net/npm/ogl@0.0.79/dist/ogl.umd.js';
            script.onload = () => resolve();
            script.onerror = () => reject(new Error('Failed to load OGL'));
            document.head.appendChild(script);
        });
    }

    createParticles() {
        const { OGL } = window;
        const {
            particleCount,
            particleSpread,
            speed,
            particleColors,
            particleBaseSize,
            moveParticlesOnHover,
            alphaParticles,
            disableRotation
        } = this.options;

        const renderer = new OGL.Renderer({ depth: false, alpha: true });
        const gl = renderer.gl;
        this.container.appendChild(gl.canvas);
        gl.clearColor(0, 0, 0, 0);

        const camera = new OGL.Camera(gl, { fov: 15 });
        camera.position.set(0, 0, 20);

        const resize = () => {
            const width = this.container.clientWidth;
            const height = this.container.clientHeight;
            renderer.setSize(width, height);
            camera.perspective({ aspect: gl.canvas.width / gl.canvas.height });
        };
        window.addEventListener('resize', resize, false);
        resize();

        this.mouse = { x: 0, y: 0 };
        
        if (moveParticlesOnHover) {
            this.container.addEventListener('mousemove', (e) => {
                const rect = this.container.getBoundingClientRect();
                this.mouse.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
                this.mouse.y = -(((e.clientY - rect.top) / rect.height) * 2 - 1);
            });
        }

        const count = particleCount;
        const positions = new Float32Array(count * 3);
        const randoms = new Float32Array(count * 4);
        const colors = new Float32Array(count * 3);

        for (let i = 0; i < count; i++) {
            let x, y, z, len;
            do {
                x = Math.random() * 2 - 1;
                y = Math.random() * 2 - 1;
                z = Math.random() * 2 - 1;
                len = x * x + y * y + z * z;
            } while (len > 1 || len === 0);
            const r = Math.cbrt(Math.random());
            positions.set([x * r, y * r, z * r], i * 3);
            randoms.set([Math.random(), Math.random(), Math.random(), Math.random()], i * 4);
            const col = this.hexToRgb(particleColors[Math.floor(Math.random() * particleColors.length)]);
            colors.set(col, i * 3);
        }

        const geometry = new OGL.Geometry(gl, {
            position: { size: 3, data: positions },
            random: { size: 4, data: randoms },
            color: { size: 3, data: colors }
        });

        const vertex = `
            attribute vec3 position;
            attribute vec4 random;
            attribute vec3 color;
            
            uniform mat4 modelMatrix;
            uniform mat4 viewMatrix;
            uniform mat4 projectionMatrix;
            uniform float uTime;
            uniform float uSpread;
            uniform float uBaseSize;
            uniform float uSizeRandomness;
            
            varying vec4 vRandom;
            varying vec3 vColor;
            
            void main() {
                vRandom = random;
                vColor = color;
                
                vec3 pos = position * uSpread;
                pos.z *= 10.0;
                
                vec4 mPos = modelMatrix * vec4(pos, 1.0);
                float t = uTime;
                mPos.x += sin(t * random.z + 6.28 * random.w) * mix(0.1, 1.5, random.x);
                mPos.y += sin(t * random.y + 6.28 * random.x) * mix(0.1, 1.5, random.w);
                mPos.z += sin(t * random.w + 6.28 * random.y) * mix(0.1, 1.5, random.z);
                
                vec4 mvPos = viewMatrix * mPos;

                if (uSizeRandomness == 0.0) {
                    gl_PointSize = uBaseSize;
                } else {
                    gl_PointSize = (uBaseSize * (1.0 + uSizeRandomness * (random.x - 0.5))) / length(mvPos.xyz);
                }

                gl_Position = projectionMatrix * mvPos;
            }
        `;

        const fragment = `
            precision highp float;
            
            uniform float uTime;
            uniform float uAlphaParticles;
            varying vec4 vRandom;
            varying vec3 vColor;
            
            void main() {
                vec2 uv = gl_PointCoord.xy;
                float d = length(uv - vec2(0.5));
                
                if(uAlphaParticles < 0.5) {
                    if(d > 0.5) {
                        discard;
                    }
                    gl_FragColor = vec4(vColor + 0.2 * sin(uv.yxx + uTime + vRandom.y * 6.28), 1.0);
                } else {
                    float circle = smoothstep(0.5, 0.4, d) * 0.8;
                    gl_FragColor = vec4(vColor + 0.2 * sin(uv.yxx + uTime + vRandom.y * 6.28), circle);
                }
            }
        `;

        const program = new OGL.Program(gl, {
            vertex,
            fragment,
            uniforms: {
                uTime: { value: 0 },
                uSpread: { value: particleSpread },
                uBaseSize: { value: particleBaseSize },
                uSizeRandomness: { value: 1 },
                uAlphaParticles: { value: alphaParticles ? 1 : 0 }
            },
            transparent: true,
            depthTest: false
        });

        this.particles = new OGL.Mesh(gl, { mode: gl.POINTS, geometry, program });
        this.renderer = renderer;
        this.camera = camera;
        this.lastTime = performance.now();
        this.elapsed = 0;
        this.speed = speed;

        this.animate();
    }

    animate() {
        this.animationFrame = requestAnimationFrame(this.animate.bind(this));
        const currentTime = performance.now();
        const delta = currentTime - this.lastTime;
        this.lastTime = currentTime;
        this.elapsed += delta * this.speed;

        this.particles.program.uniforms.uTime.value = this.elapsed * 0.001;

        if (this.options.moveParticlesOnHover) {
            this.particles.position.x = -this.mouse.x;
            this.particles.position.y = -this.mouse.y;
        } else {
            this.particles.position.x = 0;
            this.particles.position.y = 0;
        }

        if (!this.options.disableRotation) {
            this.particles.rotation.x = Math.sin(this.elapsed * 0.0002) * 0.1;
            this.particles.rotation.y = Math.cos(this.elapsed * 0.0005) * 0.15;
            this.particles.rotation.z += 0.01 * this.speed;
        }

        this.renderer.render({ scene: this.particles, camera: this.camera });
    }

    destroy() {
        if (this.animationFrame) {
            cancelAnimationFrame(this.animationFrame);
        }
        if (this.container && this.renderer) {
            this.container.removeChild(this.renderer.gl.canvas);
        }
    }
}